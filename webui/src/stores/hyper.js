import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { execCommand, sanitize, isKSU } from '@/helpers/shell'

const MOD = '/data/adb/modules/hypercore'
const LOG = '/sdcard/Android/hypercore.log'
const GL_PERM = '/data/adb/hypercore/gamelist.txt'
const GL_MOD = `${MOD}/gamelist.txt`
const GL_SD = '/sdcard/Android/gamelist.txt'

export const useHyperStore = defineStore('hyper', () => {
  
  const daemonPid = ref('')
  const activeProfile = ref('—')
  const thermalTier = ref('—')

  const cpuLittle = ref('—')
  const cpuBig = ref('—')
  const cpuGov = ref('—')
  const cpuCores = ref('—')
  const gpuInfo = ref('—')
  const sysLoad = ref('—')

  const ramUsage = ref('—')
  const ramPercent = ref(0)
  const zramUsage = ref('—')
  const zramPercent = ref(0)
  const ioInfo = ref('—')
  const vmInfo = ref('—')

  const cpuTemp = ref(0)
  const batTemp = ref(0)
  const gpuTemp = ref(0)
  const chgTemp = ref(0)
  const batStatus = ref('—')
  const batLevel = ref('')
  const batRate = ref('—')
  const batVolt = ref('—')
  const batteryCycles = ref(0)
  const batHealth = ref('Good')
  const batCapFull = ref('5350 mAh')
  const batTech = ref('Li-poly')

  /* Charger Control */
  const chargeMode = ref(0)              /* CHARGE_MODE_* (0=OEM 1=Fast 2=Balanced 3=Safe 4=Bypass 5=Violent) */
  const chargeModeOverride = ref(false)  /* true = daemon overrode user choice due to heat */
  const chargerSupported = ref(true)     /* true = device has required charger sysfs nodes */
  const chargeCurrentMa = ref(0)         /* real-time mA from current_now */
  const chargeVoltMv = ref(0)            /* real-time mV from voltage_now */

  const thermalGuardState = computed(() => {
    if (batTemp.value >= 42) return 'Trickle Guard (1.0A)'
    if (batTemp.value >= 37) return 'Safety Balance (1.8A)'
    return 'Fast Charge (2.5A)'
  })

  const moduleVersion = ref('v6.2.0')
  const kernelVersion = ref('—')
  const chipset = ref('MediaTek MT6789 Family')
  const uptime = ref('—')
  const uptimeSec = ref(0)
  let uptimeInterval = null

  const games = ref([])
  const logs = ref('Loading...')
  const loading = ref(false)
  const isLogsActive = ref(false)
  let isRefreshing = false

  function setLogsActive(active) {
    isLogsActive.value = active
  }

  const isRunning = computed(() => daemonPid.value.length > 0)

  function normTemp(raw) {
    const v = parseInt(raw || 0)
    if (v > 1000) return Math.round(v / 1000)
    if (v > 100) return Math.round(v / 10)
    return v
  }

  function fmtFreq(raw) {
    return Math.round(parseInt(raw || 0) / 1000)
  }

  function fmtUptime(totalSec) {
    const s = Math.floor(totalSec || 0)
    if (!s) return '—'
    const d = Math.floor(s / 86400)
    const h = Math.floor((s % 86400) / 3600)
    const m = Math.floor((s % 3600) / 60)
    const sec = Math.floor(s % 60)

    if (d > 0) return `${d}d ${h}h ${m}m ${sec}s`
    if (h > 0) return `${h}h ${m}m ${sec}s`
    return `${m}m ${sec}s`
  }

  function startUptimeTicker() {
    if (uptimeInterval) return
    uptimeInterval = setInterval(() => {
      if (uptimeSec.value > 0) {
        uptimeSec.value++
        uptime.value = fmtUptime(uptimeSec.value)
      }
    }, 1000)
  }

  function stopUptimeTicker() {
    if (uptimeInterval) {
      clearInterval(uptimeInterval)
      uptimeInterval = null
    }
  }

  let cpuGpuInterval = null
  let ramBatInterval = null
  let chargerInterval = null

  async function pollCpuGpu() {
    try {
      const res = await execCommand(
        `echo "CT:$(cat /sys/class/thermal/thermal_zone16/temp 2>/dev/null || cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)";` +
        `echo "GOV:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)";` +
        `echo "GPU:$(cat /sys/module/ged/parameters/gpu_loading 2>/dev/null):$(cat /sys/module/ged/parameters/gpu_bottom_freq 2>/dev/null)";` +
        `echo "STAT:$(cat /data/adb/modules/hypercore/status.json 2>/dev/null || cat /data/adb/hypercore/status.json 2>/dev/null)";` +
        `echo "PID:$(cat /data/adb/modules/hypercore/hypercore.pid 2>/dev/null || pidof libhypercore.so || pidof hypercore 2>/dev/null)"`
      )
      if (!res) return
      const kv = {}
      res.trim().split('\n').forEach(line => {
        const i = line.indexOf(':')
        if (i > 0) kv[line.substring(0, i)] = line.substring(i + 1)
      })
      if (kv.CT) cpuTemp.value = normTemp(kv.CT)
      if (kv.GOV) cpuGov.value = kv.GOV.trim()
      if (kv.GPU) {
        const p = kv.GPU.split(':')
        gpuInfo.value = `${p[0] || '0'}% / ${fmtFreq(p[1])} MHz floor`
      }
      if (kv.STAT && kv.STAT.includes('"status":"ok"')) {
        try {
          const s = JSON.parse(kv.STAT.substring(kv.STAT.indexOf('{')))
          if (s.pid) daemonPid.value = String(s.pid).split(' ')[0]
          if (s.profile) activeProfile.value = s.profile
          if (typeof s.thermal_tier !== 'undefined') thermalTier.value = `Tier ${s.thermal_tier}`
          if (s.battery_cycles && parseInt(s.battery_cycles) > 0) batteryCycles.value = parseInt(s.battery_cycles)
        } catch {}
      } else if (kv.PID && kv.PID.trim().length > 0) {
        daemonPid.value = kv.PID.trim().split(' ')[0]
      }
    } catch {}
  }

  async function pollRamBat() {
    try {
      const res = await execCommand(
        `echo "MEM:$(grep -E '^(MemTotal|MemAvailable):' /proc/meminfo 2>/dev/null | tr '\\n' ' ')";` +
        `echo "BS:$(cat /sys/class/power_supply/battery/status 2>/dev/null)";` +
        `echo "BT:$(cat /sys/class/power_supply/battery/temp 2>/dev/null)";` +
        `echo "BC:$(cat /sys/class/power_supply/battery/current_now 2>/dev/null)";` +
        `echo "CYC:$(cat /sys/class/power_supply/battery/cycle_count 2>/dev/null || cat /sys/class/power_supply/battery/auth_dev_batt_cycle 2>/dev/null || cat /sys/class/power_supply/bms/cycle_count 2>/dev/null)"`
      )
      if (!res) return
      const kv = {}
      res.trim().split('\n').forEach(line => {
        const i = line.indexOf(':')
        if (i > 0) kv[line.substring(0, i)] = line.substring(i + 1)
      })
      if (kv.MEM) {
        const m = kv.MEM.match(/MemTotal:\s*(\d+)\s*kB.*MemAvailable:\s*(\d+)\s*kB/)
        if (m) {
          const tot = Math.round(parseInt(m[1]) / 1024)
          const avail = Math.round(parseInt(m[2]) / 1024)
          const used = tot - avail
          ramPercent.value = Math.round((used / tot) * 100)
          ramUsage.value = `${(used / 1024).toFixed(1)} GB / ${(tot / 1024).toFixed(1)} GB (${ramPercent.value}%)`
        }
      }
      if (kv.BS) batStatus.value = kv.BS.trim()
      if (kv.BT) batTemp.value = normTemp(kv.BT)
      if (kv.BC) {
        const rawCurr = Math.abs(parseInt(kv.BC || 0))
        if (rawCurr > 0) {
          const ma = Math.round(rawCurr > 10000 ? rawCurr / 1000 : rawCurr)
          const sign = batStatus.value === 'Charging' ? '+' : '-'
          batRate.value = `${sign}${ma} mA`
        }
      }
      if (kv.CYC && parseInt(kv.CYC) > 0) batteryCycles.value = parseInt(kv.CYC)
    } catch {}
  }

  function startCardPolling() {
    if (!cpuGpuInterval) {
      /* Guard: skip poll tick if a full refresh() is already in flight to avoid
       * stale partial data overwriting values refresh() is currently updating */
      cpuGpuInterval = setInterval(() => { if (!isRefreshing) pollCpuGpu() }, 3000)
    }
    if (!ramBatInterval) {
      ramBatInterval = setInterval(() => { if (!isRefreshing) pollRamBat() }, 5000)
    }
    if (!chargerInterval) {
      chargerInterval = setInterval(() => { if (!isRefreshing) pollCharger() }, 2000)
    }
  }

  function stopCardPolling() {
    if (cpuGpuInterval) { clearInterval(cpuGpuInterval); cpuGpuInterval = null }
    if (ramBatInterval) { clearInterval(ramBatInterval); ramBatInterval = null }
    if (chargerInterval) { clearInterval(chargerInterval); chargerInterval = null }
  }

  async function refresh() {
    if (isRefreshing) return
    isRefreshing = true

    const fetchLogs = isLogsActive.value ? '1' : '0'
    const cmd = `MOD="/data/adb/modules/hypercore";
IPC=$(cat $MOD/status.json 2>/dev/null || cat /data/adb/hypercore/status.json 2>/dev/null || echo GET_STATUS | nc -w 1 -U /dev/hypercore.sock 2>/dev/null || echo GET_STATUS | nc -w 1 -U $MOD/hypercore.sock 2>/dev/null || echo GET_STATUS | nc -w 1 -U /data/adb/hypercore/hypercore.sock 2>/dev/null || true); echo "IPC:$IPC";
echo "PID:$(cat $MOD/hypercore.pid 2>/dev/null || pidof libhypercore.so || pidof hypercore 2>/dev/null)";
echo "CL0:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq 2>/dev/null)";
echo "CL1:$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_max_freq 2>/dev/null)";
echo "GOV:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)";
echo "CORES:$(cat /sys/devices/system/cpu/online 2>/dev/null)";
echo "GPU:$(cat /sys/module/ged/parameters/gpu_loading 2>/dev/null):$(cat /sys/module/ged/parameters/gpu_bottom_freq 2>/dev/null)";
echo "LOAD:$(read -r a _ < /proc/loadavg 2>/dev/null && echo "$a")";
echo "MEM:$(grep -E '^(MemTotal|MemAvailable|SwapTotal|SwapFree):' /proc/meminfo 2>/dev/null | tr '\n' ' ')";
echo "CT:$(cat /sys/class/thermal/thermal_zone16/temp 2>/dev/null || cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)";
echo "BT:$(cat /sys/class/power_supply/battery/temp 2>/dev/null)";
echo "BS:$(cat /sys/class/power_supply/battery/status 2>/dev/null)";
echo "BL:$(cat /sys/class/power_supply/battery/capacity 2>/dev/null)";
echo "BC:$(cat /sys/class/power_supply/battery/current_now 2>/dev/null)";
echo "BV:$(cat /sys/class/power_supply/battery/voltage_now 2>/dev/null)";
echo "BH:$(cat /sys/class/power_supply/battery/health 2>/dev/null || cat /sys/class/power_supply/bms/health 2>/dev/null)";
echo "BFC:$(cat /sys/class/power_supply/battery/charge_full 2>/dev/null || cat /sys/class/power_supply/bms/charge_full 2>/dev/null)";
echo "BTEC:$(cat /sys/class/power_supply/battery/technology 2>/dev/null || cat /sys/class/power_supply/bms/technology 2>/dev/null)";
echo "IO:$(cat /sys/block/mmcblk0/queue/scheduler /sys/block/sda/queue/scheduler 2>/dev/null | head -1 | grep -oP '\\[\\K[^]]+'):256";
echo "SW:$(cat /proc/sys/vm/swappiness 2>/dev/null):100";
echo "UP:$(read -r u _ < /proc/uptime 2>/dev/null && echo "$u")";
echo "KV:$(uname -r 2>/dev/null)";
echo "VER:$(grep '^version=' /data/adb/modules/hypercore/module.prop 2>/dev/null | cut -d= -f2 || echo v6.2.0)";
echo "===GL===";
cat ${GL_PERM} 2>/dev/null || cat $MOD/gamelist.txt 2>/dev/null || true;
if [ "${fetchLogs}" = "1" ]; then echo "===LOG==="; tail -n 35 ${LOG} 2>/dev/null || tail -n 35 /data/adb/hypercore/hypercore.log 2>/dev/null || true; fi`

    try {
      const res = await execCommand(cmd)
      if (!res) return

      const sections = res.split('===GL===')
      const kvBlock = sections[0] || ''
      const rest = (sections[1] || '').split('===LOG===')
      const gameBlock = rest[0] || ''
      const logBlock = rest[1] || ''

      const kv = {}
      kvBlock.trim().split('\n').forEach(line => {
        const i = line.indexOf(':')
        if (i > 0) kv[line.substring(0, i)] = line.substring(i + 1)
      })

      if (kv.VER) moduleVersion.value = kv.VER.trim()
      if (kv.KV && kv.KV.trim().length > 0) kernelVersion.value = kv.KV.trim()

      let ipcSetThermal = false
      if (kv.IPC && kv.IPC.includes('"status":"ok"')) {
        try {
          const ipcData = JSON.parse(kv.IPC.substring(kv.IPC.indexOf('{')))
          if (ipcData.pid) daemonPid.value = String(ipcData.pid).split(' ')[0]
          if (ipcData.profile) activeProfile.value = ipcData.profile
          if (typeof ipcData.thermal_tier !== 'undefined') thermalTier.value = `Tier ${ipcData.thermal_tier}`
          if (ipcData.battery_cycles > 0) batteryCycles.value = ipcData.battery_cycles
          if (ipcData.gpu_temp > 0) gpuTemp.value = ipcData.gpu_temp
          if (ipcData.chg_temp > 0) chgTemp.value = ipcData.chg_temp
          if (typeof ipcData.charge_mode !== 'undefined') chargeMode.value = ipcData.charge_mode
          if (typeof ipcData.charge_thermal_override !== 'undefined') chargeModeOverride.value = !!ipcData.charge_thermal_override
          if (typeof ipcData.charger_supported !== 'undefined') chargerSupported.value = !!ipcData.charger_supported
          ipcSetThermal = true
        } catch {}
      }
      
      if (!ipcSetThermal) {
        const rawPid = (kv.PID || '').trim().split(' ')[0]
        if (rawPid && rawPid.length > 0) daemonPid.value = rawPid

        const desc = kv.DESC || ''
        const m = desc.match(/\[Active:\s*([^\]]+)\]/)
        if (m) activeProfile.value = m[1].trim()
      }

      if (kv.CL0) {
        const p = kv.CL0.split(':')
        cpuLittle.value = `${fmtFreq(p[0])} MHz [${fmtFreq(p[1])}–${fmtFreq(p[2])}]`
      }
      if (kv.CL1) {
        const p = kv.CL1.split(':')
        cpuBig.value = `${fmtFreq(p[0])} MHz [${fmtFreq(p[1])}–${fmtFreq(p[2])}]`
      }
      cpuGov.value = (kv.GOV || 'schedutil').trim()
      cpuCores.value = (kv.CORES || '0-7').trim()

      if (kv.GPU) {
        const p = kv.GPU.split(':')
        gpuInfo.value = `${p[0] || '0'}% / ${fmtFreq(p[1])} MHz floor`
      }

      sysLoad.value = kv.LOAD || '0.00'
      if (kv.UP) {
        const parsedSec = Math.floor(parseFloat(kv.UP.trim()))
        if (parsedSec > 0) {
          if (!uptimeInterval) {
            // First time: seed the counter from /proc/uptime and start the ticker
            uptimeSec.value = parsedSec
            uptime.value = fmtUptime(parsedSec)
            startUptimeTicker()
          }
          // If ticker is already running, leave it alone — it increments per second on its own
        }
      }

      if (kv.MEM) {
        const memMatch = kv.MEM.match(/MemTotal:\s*(\d+)\s*kB.*MemAvailable:\s*(\d+)\s*kB/)
        const swapMatch = kv.MEM.match(/SwapTotal:\s*(\d+)\s*kB.*SwapFree:\s*(\d+)\s*kB/)

        if (memMatch) {
          const tot = Math.round(parseInt(memMatch[1]) / 1024)
          const avail = Math.round(parseInt(memMatch[2]) / 1024)
          const used = tot - avail
          ramPercent.value = Math.round((used / tot) * 100)
          ramUsage.value = `${(used / 1024).toFixed(1)} GB / ${(tot / 1024).toFixed(1)} GB (${ramPercent.value}%)`
        }

        if (swapMatch) {
          const tot = Math.round(parseInt(swapMatch[1]) / 1024)
          const free = Math.round(parseInt(swapMatch[2]) / 1024)
          const used = tot - free
          zramPercent.value = tot > 0 ? Math.round((used / tot) * 100) : 0
          zramUsage.value = `${(used / 1024).toFixed(1)} GB / ${(tot / 1024).toFixed(1)} GB (${zramPercent.value}%)`
        }
      }

      cpuTemp.value = normTemp(kv.CT)
      batTemp.value = normTemp(kv.BT)

      batStatus.value = (kv.BS || 'Unknown').trim()
      batLevel.value = (kv.BL || '').trim()

      const rawCurr = Math.abs(parseInt(kv.BC || 0))
      const rawVolt = parseInt(kv.BV || 0)
      if (rawCurr > 0) {
        const ma = Math.round(rawCurr > 10000 ? rawCurr / 1000 : rawCurr)
        const sign = batStatus.value === 'Charging' ? '+' : '-'
        batRate.value = `${sign}${ma} mA`
      } else {
        batRate.value = '—'
      }

      if (rawVolt > 0) {
        const v = (rawVolt > 10000 ? rawVolt / 1000000 : rawVolt / 1000).toFixed(2)
        batVolt.value = `${v} V`
      } else {
        batVolt.value = '—'
      }

      if (kv.BH) batHealth.value = kv.BH.trim() || 'Good'
      if (kv.BTEC) batTech.value = kv.BTEC.trim() || 'Li-poly'
      if (kv.BFC) {
        const rawFc = parseInt(kv.BFC || 0)
        if (rawFc > 0) {
          const mah = Math.round(rawFc > 10000 ? rawFc / 1000 : rawFc)
          batCapFull.value = `${mah} mAh`
        }
      }

      if (kv.IO) {
        const p = kv.IO.split(':')
        ioInfo.value = `${p[0] || 'none'} / ${p[1] || '256'} KB`
      }
      if (kv.SW) {
        const p = kv.SW.split(':')
        vmInfo.value = `${p[0] || '15'} / ${p[1] || '100'}`
      }

      if (!ipcSetThermal) {
        const logLines = logBlock.trim().split('\n')
        let tier = '—'
        for (let i = logLines.length - 1; i >= 0; i--) {
          if (logLines[i].indexOf('Thermal Tier:') !== -1) {
            if (logLines[i].indexOf('T3') !== -1) tier = 'Tier 3 (Protection)'
            else if (logLines[i].indexOf('T2') !== -1) tier = 'Tier 2 (Warm)'
            else if (logLines[i].indexOf('T1') !== -1) tier = 'Tier 1 (Normal)'
            else tier = 'Tier 0 (Optimal)'
            break
          }
        }
        if (tier === '—' && cpuTemp.value > 0) {
          if (cpuTemp.value >= 75) tier = 'Tier 3 (Protection)'
          else if (cpuTemp.value >= 65) tier = 'Tier 2 (Warm)'
          else if (cpuTemp.value >= 58) tier = 'Tier 1 (Normal)'
          else tier = 'Tier 0 (Optimal)'
        }
        thermalTier.value = tier
      }

      const parsedPkgs = []
      gameBlock.trim().split('\n').forEach(line => {
        const trimmed = line.trim().replace(/\r/g, '')
        if (trimmed.length > 0 && trimmed[0] !== '#') {
          const parts = trimmed.split(':')
          const pkg = parts[0].trim()
          const profile = parts[1] ? parts[1].trim() : 'GAMING'
          if (pkg && !parsedPkgs.some(p => p.pkg === pkg)) {
            parsedPkgs.push({ pkg, profile })
          }
        }
      })
      games.value = parsedPkgs

      const trimmed = logBlock.trim()
      if (trimmed) logs.value = trimmed

    } catch (err) {
      console.error('refresh:', err)
    } finally {
      isRefreshing = false
    }
  }

  async function pollCharger() {
    try {
      const res = await execCommand(
        `echo "CC:$(cat /sys/class/power_supply/battery/current_now 2>/dev/null)";` +
        `echo "CV:$(cat /sys/class/power_supply/battery/voltage_now 2>/dev/null)";` +
        `echo "CM:$(cat /data/adb/modules/hypercore/status.json 2>/dev/null || cat /data/adb/hypercore/status.json 2>/dev/null || echo GET_CHARGE_MODE | nc -w 1 -U /dev/hypercore.sock 2>/dev/null || true)"`
      )
      if (!res) return
      const kv = {}
      res.trim().split('\n').forEach(line => {
        const i = line.indexOf(':')
        if (i > 0) kv[line.substring(0, i)] = line.substring(i + 1)
      })
      if (kv.CC) {
        const rawMa = Math.abs(parseInt(kv.CC || 0))
        chargeCurrentMa.value = rawMa > 10000 ? Math.round(rawMa / 1000) : rawMa
      }
      if (kv.CV) {
        const rawUv = parseInt(kv.CV || 0)
        chargeVoltMv.value = rawUv > 10000 ? Math.round(rawUv / 1000) : rawUv
      }
      /* Parse charge_mode from status.json or GET_CHARGE_MODE IPC response */
      if (kv.CM && kv.CM.includes('"status":"ok"')) {
        try {
          const cm = JSON.parse(kv.CM.substring(kv.CM.indexOf('{')))
          if (typeof cm.charge_mode !== 'undefined') chargeMode.value = cm.charge_mode
          if (typeof cm.charge_thermal_override !== 'undefined') chargeModeOverride.value = !!cm.charge_thermal_override
          if (typeof cm.charger_supported !== 'undefined') chargerSupported.value = !!cm.charger_supported
        } catch {}
      }
    } catch {}
  }

  async function setChargeMode(mode) {
    loading.value = true
    try {
      const m = parseInt(mode)

      /* 1. Persist directly to charge_mode.conf as instant disk fallback */
      const diskCmd = `MOD="/data/adb/modules/hypercore"; mkdir -p $MOD 2>/dev/null; echo ${m} > $MOD/charge_mode.conf 2>/dev/null || echo ${m} > /data/adb/hypercore/charge_mode.conf 2>/dev/null || true`
      execCommand(diskCmd).catch(() => {})

      /* 2. Send SET_CHARGE_MODE IPC to UNIX domain socket */
      const ipcCmd = `MOD="/data/adb/modules/hypercore"; echo SET_CHARGE_MODE:${m} | nc -w 2 -U /dev/hypercore.sock 2>/dev/null || echo SET_CHARGE_MODE:${m} | nc -w 2 -U $MOD/hypercore.sock 2>/dev/null || echo SET_CHARGE_MODE:${m} | nc -w 2 -U /data/adb/hypercore/hypercore.sock 2>/dev/null || true`
      const res = await execCommand(ipcCmd)

      /* Optimistically update state */
      chargeMode.value = m
      chargeModeOverride.value = false

      /* 3. If IPC returned JSON response, parse authoritative data directly */
      if (res && res.includes('"status":"ok"')) {
        try {
          const data = JSON.parse(res.substring(res.indexOf('{')))
          if (typeof data.charge_mode !== 'undefined') chargeMode.value = data.charge_mode
          if (typeof data.charger_supported !== 'undefined') chargerSupported.value = !!data.charger_supported
        } catch {}
      }

      /* 4. Wait 200ms for daemon to flush status.json before running pollCharger to avoid stale race condition */
      await new Promise(r => setTimeout(r, 200))
      await pollCharger()
      return res
    } catch (e) {
      return 'error'
    } finally {
      loading.value = false
    }
  }

  async function flushRam() {

    loading.value = true
    try {
      // Primary: request daemon via IPC (daemon handles this safely without compact_memory)
      // Fallback: drop_caches only — DO NOT write compact_memory directly;
      // it triggers synchronous kernel compaction that freezes all userland threads
      // (200-800ms D-state) and can cause ANR/soft reboot. See daemon C-1 fix.
      const cmd = `MOD="/data/adb/modules/hypercore";
echo PURGE_RAM | nc -U /dev/hypercore.sock 2>/dev/null || echo PURGE_RAM | nc -U $MOD/hypercore.sock 2>/dev/null || (sync; echo 3 > /proc/sys/vm/drop_caches 2>/dev/null; echo 60 > /proc/sys/vm/compaction_proactiveness 2>/dev/null || true)`
      await execCommand(cmd)
      await new Promise(resolve => setTimeout(resolve, 400))
      await refresh()
      return `Deep RAM & Cache purge complete (${ramUsage.value})`
    } catch (e) {
      return 'Failed to clear RAM'
    } finally {
      loading.value = false
    }
  }

  async function restartDaemon() {
    loading.value = true
    try {
      const cmd = `MOD="/data/adb/modules/hypercore";
pkill -15 -x libhypercore.so 2>/dev/null || true;
sleep 0.3;
pkill -9 -x libhypercore.so 2>/dev/null || true;
rm -f /dev/hypercore.sock $MOD/hypercore.sock $MOD/hypercore.pid 2>/dev/null || true;
nohup $MOD/system/bin/libhypercore.so >/dev/null 2>&1 &`
      await execCommand(cmd)

      let started = false
      for (let i = 0; i < 10; i++) {
        await new Promise(r => setTimeout(r, 250))
        const check = await execCommand('echo GET_STATUS | nc -w 2 -U /dev/hypercore.sock 2>/dev/null || echo GET_STATUS | nc -w 2 -U /data/adb/modules/hypercore/hypercore.sock 2>/dev/null || true')
        if (check && check.includes('"status":"ok"')) {
          started = true
          break
        }
      }

      await refresh()
      return started ? 'Daemon restarted & active' : 'Daemon start requested'
    } catch (e) {
      return 'Failed to restart daemon'
    } finally {
      loading.value = false
    }
  }

  async function exportLogs(note = '') {
    loading.value = true
    try {
      const safeNote = sanitize(note)
        .replace(/[$`\\]/g, '')
        .replace(/'/g, '')
        .slice(0, 500)
      const noteArg = safeNote ? `'${safeNote}'` : ''
      const cmd = `${MOD}/system/bin/hypercore-bugreport ${noteArg} 2>/dev/null \
        || sh ${MOD}/system/bin/hypercore-bugreport ${noteArg} 2>/dev/null`
      const res = await execCommand(cmd)
      const path = (res || '').trim().split('\n').pop()
      if (path && path !== 'ERROR' && (path.indexOf('.zip') !== -1 || path.indexOf('.tar.gz') !== -1)) {
        return path
      }
      await execCommand(`cp ${LOG} /sdcard/HyperCore_log.txt 2>/dev/null`)
      return '/sdcard/HyperCore_log.txt'
    } catch (e) {
      return ''
    } finally {
      loading.value = false
    }
  }

  async function clearLogs() {
    loading.value = true
    await execCommand(`> ${LOG} 2>/dev/null || true; > /sdcard/Android/hypercore.log 2>/dev/null || true`)
    logs.value = ''
    loading.value = false
    return 'Log cleared'
  }

  function createShortcut() {
    if (isKSU() && typeof ksu.createShortcut === 'function') {
      ksu.createShortcut()
      return 'Shortcut requested'
    }
    return 'Shortcut feature unavailable'
  }

  function addGame(rawPkg, profile = 'GAMING') {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    if (!pkg) return

    if (!games.value.some(g => g.pkg === pkg)) {
      games.value.push({ pkg, profile })
    }

    import('@/helpers/shell').then(m => m.listInstalledApps(true)).catch(() => {})

    const cmd = [
      `mkdir -p /data/adb/hypercore 2>/dev/null`,
      `for f in ${GL_PERM} ${GL_MOD} ${GL_SD}; do`,
      `  touch "$f" 2>/dev/null`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p="${pkg}" '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    echo '${pkg}:${profile}' >> "$f.tmp"`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `  fi`,
      `done`
    ].join('\n')
    execCommand(cmd)
    setTimeout(refresh, 400)
    return `Added ${pkg} (${profile})`
  }

  function removeGame(rawPkg) {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    if (!pkg) return

    games.value = games.value.filter(g => g.pkg !== pkg)

    import('@/helpers/shell').then(m => m.listInstalledApps(true)).catch(() => {})

    const cmd = [
      `for f in ${GL_PERM} ${GL_MOD} ${GL_SD}; do`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p="${pkg}" '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `  fi`,
      `done`
    ].join('\n')
    execCommand(cmd)
    setTimeout(refresh, 400)
    return `Removed ${pkg}`
  }

  function updateGameProfile(rawPkg, rawProfile) {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    const profile = sanitize(rawProfile).toUpperCase().replace(/[^A-Z_]/g, '')
    if (!pkg || !profile) return

    const existing = games.value.find(g => g.pkg === pkg)
    if (existing) {
      existing.profile = profile
    }

    const cmd = [
      `for f in ${GL_PERM} ${GL_MOD} ${GL_SD}; do`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p="${pkg}" '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    echo '${pkg}:${profile}' >> "$f.tmp"`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `  fi`,
      `done`
    ].join('\n')
    execCommand(cmd)
    setTimeout(refresh, 400)
    return `Updated ${pkg} to ${profile}`
  }

  function launchGame(rawPkg) {
    const clean = sanitize(rawPkg).split(':')[0].trim()
    if (!clean) return
    execCommand(`monkey -p ${clean} -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1`)
    return `Launched ${clean}`
  }

  return {
    daemonPid, activeProfile, thermalTier, cpuLittle, cpuBig, cpuGov, cpuCores, gpuInfo,
    sysLoad, ramUsage, ramPercent, zramUsage, zramPercent, ioInfo, vmInfo,
    cpuTemp, batTemp, gpuTemp, chgTemp, batStatus, batLevel, batRate, batVolt, batteryCycles,
    batHealth, batCapFull, batTech, thermalGuardState,
    chargeMode, chargeModeOverride, chargerSupported, chargeCurrentMa, chargeVoltMv,
    games, logs, loading,
    moduleVersion, kernelVersion, chipset, uptime,
    isRunning,
    refresh, flushRam, restartDaemon, exportLogs, clearLogs, createShortcut,
    addGame, removeGame, updateGameProfile, launchGame, setLogsActive, stopUptimeTicker,
    startCardPolling, stopCardPolling, pollCharger, setChargeMode
  }
})
