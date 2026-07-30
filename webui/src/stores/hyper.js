import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { execCommand, sanitize, isKSU } from '@/helpers/shell'

const MOD = '/data/adb/modules/tanzanite_hypercore'
const LOG = '/sdcard/Android/hypercore.log'
const LOCK_MOD = `${MOD}/.hypercore_lock`
const LOCK_SD = '/sdcard/Android/.hypercore_lock'
const GL_MOD = `${MOD}/gamelist.txt`
const GL_SD = '/sdcard/Android/gamelist.txt'

export const useHyperStore = defineStore('hyper', () => {
  /* ── Reactive state ── */
  const daemonPid = ref('')
  const activeProfile = ref('—')
  const thermalTier = ref('—')
  const cpuLittle = ref('—')
  const cpuBig = ref('—')
  const gpuInfo = ref('—')
  const sysLoad = ref('—')
  const cpuTemp = ref(0)
  const batTemp = ref(0)
  const batStatus = ref('—')
  const batLevel = ref('')
  const ioInfo = ref('—')
  const vmInfo = ref('—')
  const lockMode = ref('AUTO')
  const games = ref([])
  const logs = ref('Loading...')
  const loading = ref(false)

  /* ── Device metadata ── */
  const moduleVersion = ref('v3.3')
  const kernelVersion = ref('—')
  const chipset = ref('MediaTek Helio G99 Ultra (MT6789)')
  const androidSdk = ref('—')

  /* ── Computed ── */
  const isRunning = computed(() => daemonPid.value.length > 0)

  const thermalColor = computed(() => {
    if (thermalTier.value.includes('3')) return 'c-red'
    if (thermalTier.value.includes('2') || thermalTier.value.includes('1')) return 'c-amber'
    return 'c-green'
  })

  const tempColor = computed(() => {
    if (cpuTemp.value >= 75 || batTemp.value >= 53) return 'c-red'
    if (cpuTemp.value >= 58 || batTemp.value >= 44) return 'c-amber'
    return ''
  })

  const batColor = computed(() => {
    if (batStatus.value === 'Charging') return 'c-green'
    if (parseInt(batLevel.value) <= 15) return 'c-red'
    return ''
  })

  /* ── Helpers ── */
  function normTemp(raw) {
    const v = parseInt(raw || 0)
    if (v > 1000) return Math.round(v / 1000)
    if (v > 100) return Math.round(v / 10)
    return v
  }

  function fmtFreq(raw) {
    return Math.round(parseInt(raw || 0) / 1000)
  }

  /* ── Actions ── */
  async function fetchDeviceInfo() {
    try {
      const [kv, sdk] = await Promise.all([
        execCommand('uname -r -m 2>/dev/null'),
        execCommand('getprop ro.build.version.sdk 2>/dev/null')
      ])
      if (kv) kernelVersion.value = kv.trim()
      if (sdk) androidSdk.value = `API ${sdk.trim()}`
    } catch {}
  }

  async function refresh() {
    const cmd = [
      `PID=$(pidof hypercore 2>/dev/null); echo "PID:\${PID:-}"`,
      `echo "CL0:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq 2>/dev/null)"`,
      `echo "CL1:$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_max_freq 2>/dev/null)"`,
      `echo "GPU:$(cat /sys/module/ged/parameters/gpu_loading 2>/dev/null):$(cat /sys/module/ged/parameters/gpu_bottom_freq 2>/dev/null)"`,
      `echo "LOAD:$(cat /proc/loadavg 2>/dev/null | cut -d' ' -f1)"`,
      `echo "CT:$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)"`,
      `echo "BT:$(cat /sys/class/power_supply/battery/temp 2>/dev/null)"`,
      `echo "BS:$(cat /sys/class/power_supply/battery/status 2>/dev/null)"`,
      `echo "BL:$(cat /sys/class/power_supply/battery/capacity 2>/dev/null)"`,
      `BLK=$(ls /sys/block/ 2>/dev/null | grep -E '^(sd|mmcblk)' | head -1)`,
      `echo "IO:$(cat /sys/block/$BLK/queue/scheduler 2>/dev/null | grep -oP '\\[\\K[^]]+'):$(cat /sys/block/$BLK/queue/read_ahead_kb 2>/dev/null)"`,
      `echo "SW:$(cat /proc/sys/vm/swappiness 2>/dev/null):$(cat /proc/sys/vm/vfs_cache_pressure 2>/dev/null)"`,
      `echo "LK:$(cat ${LOCK_MOD} 2>/dev/null || cat ${LOCK_SD} 2>/dev/null || echo AUTO)"`,
      `DESC=$(grep '^description=' ${MOD}/module.prop 2>/dev/null); echo "DESC:$DESC"`,
      `echo "===GL==="`,
      `cat ${GL_MOD} 2>/dev/null || cat ${GL_SD} 2>/dev/null || true`,
      `echo "===LOG==="`,
      `tail -n 35 ${LOG} 2>/dev/null || true`
    ].join('\n')

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

      // Daemon
      daemonPid.value = (kv.PID || '').trim()

      // Profile from module.prop
      const desc = kv.DESC || ''
      const m = desc.match(/\[Active:\s*([^\]]+)\]/)
      if (m) activeProfile.value = m[1].trim()

      // CPU clusters
      if (kv.CL0) {
        const p = kv.CL0.split(':')
        cpuLittle.value = `${fmtFreq(p[0])} MHz [${fmtFreq(p[1])}–${fmtFreq(p[2])}]`
      }
      if (kv.CL1) {
        const p = kv.CL1.split(':')
        cpuBig.value = `${fmtFreq(p[0])} MHz [${fmtFreq(p[1])}–${fmtFreq(p[2])}]`
      }

      // GPU
      if (kv.GPU) {
        const p = kv.GPU.split(':')
        gpuInfo.value = `${p[0] || '0'}% / ${fmtFreq(p[1])} MHz floor`
      }

      // System load
      sysLoad.value = kv.LOAD || '0.00'

      // Temperatures
      cpuTemp.value = normTemp(kv.CT)
      batTemp.value = normTemp(kv.BT)

      // Battery
      batStatus.value = (kv.BS || 'Unknown').trim()
      batLevel.value = (kv.BL || '').trim()

      // I/O
      if (kv.IO) {
        const p = kv.IO.split(':')
        ioInfo.value = `${p[0] || 'none'} / ${p[1] || '256'} KB`
      }

      // VM
      if (kv.SW) {
        const p = kv.SW.split(':')
        vmInfo.value = `${p[0] || '15'} / ${p[1] || '100'}`
      }

      // Thermal tier
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

      // Lock mode
      lockMode.value = (kv.LK || 'AUTO').trim().toUpperCase()

      // Games
      games.value = gameBlock.trim().split('\n')
        .map(l => l.trim().replace(/\r/g, ''))
        .filter(l => l.length > 0 && l[0] !== '#')

      // Logs
      const trimmed = logBlock.trim()
      if (trimmed) logs.value = trimmed

    } catch (err) {
      console.error('refresh:', err)
    }
  }

  async function setProfile(mode) {
    const safe = sanitize(mode)
    if (safe === 'AUTO') {
      await execCommand(`rm -f ${LOCK_MOD} ${LOCK_SD}`)
    } else {
      await execCommand(`echo '${safe}' > ${LOCK_MOD}; echo '${safe}' > ${LOCK_SD}`)
    }
    lockMode.value = safe
    await refresh()
  }

  async function flushRam() {
    loading.value = true
    await execCommand('sync; echo 3 > /proc/sys/vm/drop_caches; echo 1 > /proc/sys/vm/compact_memory')
    loading.value = false
    return 'RAM cache cleared'
  }

  async function restartDaemon() {
    loading.value = true
    await execCommand(`pkill -9 -x hypercore 2>/dev/null; sleep 1; nohup ${MOD}/hypercore >/dev/null 2>&1 &`)
    loading.value = false
    setTimeout(refresh, 1500)
    return 'Daemon restarted'
  }

  async function exportLogs() {
    loading.value = true
    await execCommand(`cp ${LOG} /sdcard/HyperCore_log.txt 2>/dev/null`)
    loading.value = false
    return 'Log saved to /sdcard/HyperCore_log.txt'
  }

  function createShortcut() {
    if (isKSU() && typeof ksu.createShortcut === 'function') {
      ksu.createShortcut()
      return 'Shortcut requested'
    }
    return 'Shortcut feature unavailable'
  }

  async function addGame(pkg) {
    const safe = sanitize(pkg)
    if (!safe) return
    await execCommand(`echo '${safe}' >> ${GL_MOD}; echo '${safe}' >> ${GL_SD}`)
    await refresh()
    return `Added ${safe}`
  }

  async function removeGame(pkg) {
    const safe = sanitize(pkg)
    if (!safe) return
    await execCommand([
      `for f in ${GL_MOD} ${GL_SD}; do`,
      `  [ -f "$f" ] && { grep -Fvx '${safe}' "$f" > "$f.tmp" 2>/dev/null; mv "$f.tmp" "$f" 2>/dev/null; }`,
      `done`
    ].join('\n'))
    await refresh()
    return `Removed ${safe}`
  }

  async function launchGame(pkg) {
    const clean = sanitize(pkg).split(':')[0]
    if (!clean) return
    await execCommand(`monkey -p ${clean} -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1`)
    return `Launched ${clean}`
  }

  return {
    daemonPid, activeProfile, thermalTier, cpuLittle, cpuBig, gpuInfo,
    sysLoad, cpuTemp, batTemp, batStatus, batLevel, ioInfo, vmInfo,
    lockMode, games, logs, loading,
    moduleVersion, kernelVersion, chipset, androidSdk,
    isRunning, thermalColor, tempColor, batColor,
    fetchDeviceInfo, refresh, setProfile, flushRam, restartDaemon, exportLogs, createShortcut,
    addGame, removeGame, launchGame
  }
})
