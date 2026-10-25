import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { execCommand, sanitize, isKSU, base64EncodeUtf8, shellQuote } from '@/helpers/shell'

const MOD = '/data/adb/modules/hypercore'
const DATA = '/data/adb/hypercore'
// Telemetry lives under /data/adb (root-only), not /sdcard where any app with
// storage access could read usage history.
const LOG = '/data/adb/hypercore/hypercore.log'
const LOG_LEGACY = '/sdcard/Android/hypercore.log'
const GL_PERM = '/data/adb/hypercore/gamelist.txt'
const GL_STAMP = '/data/adb/hypercore/.gamelist_autodetected'

/* Whitespace-tolerant {"status":"ok"} check. The daemon emits compact JSON
 * but status.json round-trips and future pretty-printing must not flip the
 * UI into its dead fallback path. */
function okJson(s) {
  return /"status"\s*:\s*"ok"/.test(s || '')
}

const GAME_PROFILES = ['GAMING', 'GAMING_MOBA', 'INTERACTIVE', 'SLEEP']

/* Canonicalize a profile the way the daemon parser does, so what the UI
 * reads, shows and writes is always one of the four <select> values. Raw
 * file content like "gaming", "SAVER" or "Gaming MOBA" used to render a
 * blank select and round-trip back to disk verbatim. */
function normProfile(raw) {
  const norm = String(raw || 'GAMING').toUpperCase().replace(/[\s-]+/g, '_').replace(/[^A-Z_]/g, '')
  if (norm === 'GAMING' || norm === 'GAMING_MOBA' || norm === 'MOBA') return norm === 'MOBA' ? 'GAMING_MOBA' : norm
  if (norm === 'INTERACTIVE' || norm === 'BALANCED') return 'INTERACTIVE'
  if (norm === 'SLEEP' || norm === 'SAVER') return 'SLEEP'
  return 'GAMING'
}

export const useHyperStore = defineStore('hyper', () => {
  
  const daemonPid = ref('')
  const activeProfile = ref('—')

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
  const zramConfiguredMb = ref(null) /* null = Stock (no zram.conf), 0 = Off, else pool size in MB */
  const zramLiveMb = ref(0)          /* live /sys/block/zram0/disksize in MB */
  const zramComp = ref('')           /* live compressor, e.g. lz4 */
  const ioInfo = ref('—')
  const vmInfo = ref('—')

  const cpuTemp = ref(0)
  const batTemp = ref(0)
  const gpuTemp = ref(0)
  const batStatus = ref('—')
  const batLevel = ref('')


  const moduleVersion = ref('')
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

  // Card polling: the single refresh() syscall covers every key; no separate intervals.
  function startCardPolling() {}
  function stopCardPolling() {}

  async function refresh() {
    if (isRefreshing) return
    isRefreshing = true

    const fetchLogs = isLogsActive.value ? '1' : '0'
    const cmd = `MOD="/data/adb/modules/hypercore";
IPC=$( (echo GET_STATUS | nc -w 1 -U /dev/hypercore.sock 2>/dev/null || echo GET_STATUS | nc -w 1 -U /data/adb/hypercore/hypercore.sock 2>/dev/null) || (pidof libhypercore.so >/dev/null 2>&1 && (cat /data/adb/hypercore/status.json 2>/dev/null || cat /dev/hypercore_status.json 2>/dev/null)) || true); echo "IPC:$IPC";
echo "PID:$(pidof libhypercore.so 2>/dev/null || pidof hypercore 2>/dev/null || (P=$(cat /data/adb/hypercore/hypercore.pid 2>/dev/null) && [ -n "$P" ] && [ -d "/proc/$P" ] && grep -q libhypercore "/proc/$P/cmdline" 2>/dev/null && echo "$P") || true)";
echo "CL0:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq 2>/dev/null)";
echo "CL1:$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_cur_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_min_freq 2>/dev/null):$(cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_max_freq 2>/dev/null)";
echo "GOV:$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)";
echo "CORES:$(cat /sys/devices/system/cpu/online 2>/dev/null)";
echo "GPU:$(cat /sys/module/ged/parameters/gpu_loading 2>/dev/null):$(cat /sys/module/ged/parameters/gpu_bottom_freq 2>/dev/null)";
echo "GCUR:$(cat /sys/class/devfreq/13000000.mali/cur_freq 2>/dev/null || cat /sys/class/devfreq/soc:mali/cur_freq 2>/dev/null)";
echo "GMAX:$(cat /sys/class/devfreq/13000000.mali/max_freq 2>/dev/null || cat /sys/class/devfreq/soc:mali/max_freq 2>/dev/null)";
echo "LOAD:$(read -r a _ < /proc/loadavg 2>/dev/null && echo "$a")";
echo "MEM:$(grep -E '^(MemTotal|MemAvailable|SwapTotal|SwapFree):' /proc/meminfo 2>/dev/null | tr '\n' ' ')";
echo "CT:$(cat /sys/class/thermal/thermal_zone16/temp 2>/dev/null || cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)";
echo "BT:$(cat /sys/class/power_supply/battery/temp 2>/dev/null)";
echo "BS:$(cat /sys/class/power_supply/battery/status 2>/dev/null)";
echo "BL:$(cat /sys/class/power_supply/battery/capacity 2>/dev/null)";
echo "IO:$(cat /sys/block/mmcblk0/queue/scheduler /sys/block/sda/queue/scheduler 2>/dev/null | head -1 | sed -e 's/.*\\[\\([^]]*\\)\\].*/\\1/' | awk '{print $1}'):256";
echo "SW:$(cat /proc/sys/vm/swappiness 2>/dev/null):100";
echo "ZC:$(grep ^size_mb= /data/adb/hypercore/zram.conf 2>/dev/null | cut -d= -f2)";
echo "ZD:$(cat /sys/block/zram0/disksize 2>/dev/null)";
echo "ZCP:$(cat /sys/block/zram0/comp_algorithm 2>/dev/null)";
echo "UP:$(read -r u _ < /proc/uptime 2>/dev/null && echo "$u")";
echo "KV:$(uname -r 2>/dev/null)";
echo "VER:$(grep '^version=' /data/adb/modules/hypercore/module.prop 2>/dev/null | cut -d= -f2 || grep '^version=' ${MOD}/module.prop 2>/dev/null | cut -d= -f2 || true)";
echo "DESC:$(grep '^description=' /data/adb/modules/hypercore/module.prop 2>/dev/null | sed 's/^description=//' || true)";
echo "===GL===";
cat ${GL_PERM} 2>/dev/null || true;
if [ "${fetchLogs}" = "1" ]; then echo "===LOG==="; tail -n 35 ${LOG} 2>/dev/null || tail -n 35 ${LOG_LEGACY} 2>/dev/null || true; fi`

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

      const rawPid = (kv.PID || '').trim().split(' ')[0]
      const isDaemonAlive = rawPid.length > 0 && /^\d+$/.test(rawPid)
      if (isDaemonAlive) {
        daemonPid.value = rawPid
      } else {
        daemonPid.value = ''
        activeProfile.value = 'Stopped'
      }

      let ipcSuccess = false
      if (isDaemonAlive && kv.IPC && okJson(kv.IPC)) {
        try {
          const ipcData = JSON.parse(kv.IPC.substring(kv.IPC.indexOf('{')))
          if (ipcData.profile) activeProfile.value = ipcData.profile
          if (ipcData.gpu_temp > 0) gpuTemp.value = ipcData.gpu_temp
          ipcSuccess = true
        } catch {}
      }
      
      if (isDaemonAlive && !ipcSuccess) {
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

      if (kv.GCUR && kv.GMAX) {
        // devfreq cur/max_freq are in Hz (unlike cpufreq kHz) — normalize to MHz
        gpuInfo.value = `${Math.round(parseInt(kv.GCUR || 0) / 1000000)}/${Math.round(parseInt(kv.GMAX || 0) / 1000000)} MHz`
      } else if (kv.GPU) {
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

      if (kv.IO) {
        const p = kv.IO.split(':')
        ioInfo.value = `${p[0] || 'none'} / ${p[1] || '256'} KB`
      }
      if (kv.SW) {
        const p = kv.SW.split(':')
        vmInfo.value = `${p[0] || '15'} / ${p[1] || '100'}`
      }
      if (typeof kv.ZC !== 'undefined') {
        const raw = (kv.ZC || '').trim()
        if (!raw) zramConfiguredMb.value = null
        else {
          const n = parseInt(raw)
          zramConfiguredMb.value = isNaN(n) ? null : n
        }
      }
      if (typeof kv.ZD !== 'undefined') {
        const bytes = parseInt((kv.ZD || '').trim())
        zramLiveMb.value = bytes > 0 ? Math.round(bytes / 1048576) : 0
      }
      if (typeof kv.ZCP !== 'undefined') {
        const m = (kv.ZCP || '').match(/\[([^\]]+)\]/)
        zramComp.value = m ? m[1] : (kv.ZCP || '').trim().split(/\s+/).pop() || ''
      }

      const parsedPkgs = []
      gameBlock.trim().split('\n').forEach(line => {
        const trimmed = line.trim().replace(/\r/g, '')
        if (trimmed.length > 0 && trimmed[0] !== '#') {
          const parts = trimmed.split(':')
          const pkg = parts[0].trim()
          const profile = normProfile(parts[1] ? parts[1].trim() : 'GAMING')
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

  async function flushRam() {

    loading.value = true
    try {
      const cmd = `echo PURGE_RAM | nc -U /dev/hypercore.sock 2>/dev/null || echo PURGE_RAM | nc -U /data/adb/hypercore/hypercore.sock 2>/dev/null || (sync; echo 3 > /proc/sys/vm/drop_caches 2>/dev/null; echo 60 > /proc/sys/vm/compaction_proactiveness 2>/dev/null || true)`
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

  async function setZramSize(mode) {
    loading.value = true
    try {
      let cmd = ''
      let next = null
      if (mode === 'stock') {
        cmd = 'rm -f /data/adb/hypercore/zram.conf 2>/dev/null || true'
        next = null
      } else {
        const n = parseInt(mode)
        if (isNaN(n) || (n !== 0 && n !== 2048 && n !== 4096)) return 'invalid'
        cmd = `mkdir -p /data/adb/hypercore 2>/dev/null; echo 'size_mb=${n}' > /data/adb/hypercore/zram.conf 2>/dev/null; chmod 600 /data/adb/hypercore/zram.conf 2>/dev/null || true`
        next = n
      }
      await execCommand(cmd)
      zramConfiguredMb.value = next
      await new Promise(r => setTimeout(r, 200))
      await refresh()
      return next === null ? 'ZRAM back to Stock — reboot to apply' : `ZRAM set to ${next === 0 ? 'Off' : (next / 1024) + ' GB'} — reboot to apply`
    } catch (e) {
      return 'Failed to save ZRAM setting'
    } finally {
      loading.value = false
    }
  }

  async function rebootDevice() {
    try {
      await execCommand('reboot 2>/dev/null || svc power reboot 2>/dev/null || setprop sys.powerctl reboot 2>/dev/null', 3000)
    } catch {}
    return 'Rebooting…'
  }

  async function restartDaemon() {    loading.value = true
    try {
      const cmd = `MOD="/data/adb/modules/hypercore";
pkill -15 -x libhypercore.so 2>/dev/null || true;
sleep 0.3;
pkill -9 -x libhypercore.so 2>/dev/null || true;
rm -f /dev/hypercore.sock /data/adb/hypercore/hypercore.sock /data/adb/hypercore/hypercore.pid $MOD/hypercore.sock $MOD/hypercore.pid 2>/dev/null || true;
nohup $MOD/system/bin/libhypercore.so >/dev/null 2>&1 &`
      await execCommand(cmd)

      let started = false
      for (let i = 0; i < 10; i++) {
        await new Promise(r => setTimeout(r, 250))
        const check = await execCommand('echo GET_STATUS | nc -w 2 -U /dev/hypercore.sock 2>/dev/null || echo GET_STATUS | nc -w 2 -U /data/adb/hypercore/hypercore.sock 2>/dev/null || true')
        if (check && okJson(check)) {
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
      const b64Note = note && note.trim() ? base64EncodeUtf8(note.trim().slice(0, 1000)) : ''
      const noteArg = b64Note ? `'${b64Note}'` : "''"
      const cmd = `${MOD}/system/bin/hypercore-bugreport ${noteArg} 2>/dev/null \
        || sh ${MOD}/system/bin/hypercore-bugreport ${noteArg} 2>/dev/null`
      const res = await execCommand(cmd, 30000)
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
    await execCommand(`> ${LOG} 2>/dev/null || true`)
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

  async function addGame(rawPkg, profile = 'GAMING') {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    const prof = normProfile(profile)
    if (!pkg) return 'invalid package'

    if (!games.value.some(g => g.pkg === pkg)) {
      games.value.push({ pkg, profile: prof })
    } else {
      const existing = games.value.find(g => g.pkg === pkg)
      if (existing) existing.profile = prof
    }

    import('@/helpers/shell').then(m => m.listInstalledApps(true)).catch(() => {})

    const cmd = [
      `mkdir -p /data/adb/hypercore 2>/dev/null`,
      `for f in ${GL_PERM}; do`,
      `  touch "$f" 2>/dev/null`,
      `  chmod 600 "$f" 2>/dev/null || true`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p=${shellQuote(pkg)} '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    echo ${shellQuote(pkg + ':' + prof)} >> "$f.tmp"`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `    chmod 600 "$f" 2>/dev/null || true`,
      `  fi`,
      `done`
    ].join('\n')
    try {
      await execCommand(cmd)
    } catch (e) {
      await refresh()
      return `Failed to save ${pkg} (${e.message || e})`
    }
    await refresh()
    return `Added ${pkg} (${prof})`
  }

  async function removeGame(rawPkg) {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    if (!pkg) return

    games.value = games.value.filter(g => g.pkg !== pkg)

    import('@/helpers/shell').then(m => m.listInstalledApps(true)).catch(() => {})

    const cmd = [
      `for f in ${GL_PERM}; do`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p=${shellQuote(pkg)} '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `    chmod 600 "$f" 2>/dev/null || true`,
      `  fi`,
      `done`
    ].join('\n')
    try {
      await execCommand(cmd)
    } catch (e) {
      await refresh()
      return `Failed to remove ${pkg} (${e.message || e})`
    }
    await refresh()
    return `Removed ${pkg}`
  }

  async function updateGameProfile(rawPkg, rawProfile) {
    const pkg = sanitize(rawPkg).split(':')[0].trim()
    const profile = normProfile(rawProfile)
    if (!pkg) return

    const existing = games.value.find(g => g.pkg === pkg)
    if (existing) {
      existing.profile = profile
    }

    const cmd = [
      `for f in ${GL_PERM}; do`,
      `  if [ -f "$f" ]; then`,
      `    awk -F: -v p=${shellQuote(pkg)} '$1 != p' "$f" > "$f.tmp" 2>/dev/null`,
      `    echo ${shellQuote(pkg + ':' + profile)} >> "$f.tmp"`,
      `    mv "$f.tmp" "$f" 2>/dev/null`,
      `    chmod 600 "$f" 2>/dev/null || true`,
      `  fi`,
      `done`
    ].join('\n')
    try {
      await execCommand(cmd)
    } catch (e) {
      await refresh()
      return `Failed to update ${pkg} (${e.message || e})`
    }
    await refresh()
    return `Updated ${pkg} to ${profile}`
  }

  async function autoDetectGames() {
    loading.value = true
    try {
      const scanCmd = `pm list packages -3 2>/dev/null | cut -d: -f2 | grep -iE 'game|legend|pubg|mihoyo|genshin|honkai|freefire|roblox|activision|shooter|mojang|minecraft|supercell|brawl|clash|garena|stumble|pokemon|wanda|maleo|konami|krafton|netmarble|nexon|ea\\.gp|riotgames|square_enix|bandainamco|gameloft|zynga|rovio|miniclip|yostar|ubisoft|subwaysurf|bussimulator|carxtech|slither|angrybirds|asphalt|shadowfight|realracing|needforspeed|efootball|nintendo|sega|squareenix|capcom|kiloo|innersloth|levelinfinite' 2>/dev/null`
      const out = await execCommand(scanCmd)
      if (!out || !out.trim()) {
        return 'No installed games detected'
      }

      const found = out.trim().split('\n')
        .map(l => l.trim().replace(/[^A-Za-z0-9._]/g, ''))
        .filter(Boolean)
      const existing = new Set(games.value.map(g => g.pkg))
      const toAdd = found.filter(p => !existing.has(p))

      if (toAdd.length === 0) {
        return 'All detected games are already in the list'
      }

      // Replace-then-append per package instead of a blind append: the disk
      // file can hold duplicates the list view hides but the daemon still
      // reads (first match wins), so appending again would pile more on.
      const upsert = toAdd.map(pkg =>
        `awk -F: -v p=${shellQuote(pkg)} '$1 != p' "$f" > "$f.tmp" 2>/dev/null && echo ${shellQuote(pkg + ':GAMING')} >> "$f.tmp" && mv "$f.tmp" "$f" 2>/dev/null && chmod 600 "$f" 2>/dev/null || true`
      ).join('\n')
      const cmd = [
        `mkdir -p /data/adb/hypercore 2>/dev/null`,
        `for f in ${GL_PERM}; do`,
        `  touch "$f" 2>/dev/null`,
        `  chmod 600 "$f" 2>/dev/null || true`,
        `  if [ -f "$f" ]; then`,
        ...upsert.split('\n').map(l => `  ${l}`),
        `  fi`,
        `done`,
        // An explicit user scan counts as a detection pass, so the daemon
        // does not "rediscover" (and resurrect) entries on next restart.
        `touch ${GL_STAMP} 2>/dev/null || true`
      ].join('\n')
      await execCommand(cmd)
      await new Promise(r => setTimeout(r, 300))
      await refresh()
      return `Auto-detected and added ${toAdd.length} game${toAdd.length > 1 ? 's' : ''}`
    } catch {
      return 'Failed to auto-detect games'
    } finally {
      loading.value = false
    }
  }

  function launchGame(rawPkg) {
    const clean = sanitize(rawPkg).split(':')[0].trim()
    if (!clean) return
    execCommand(`monkey -p ${shellQuote(clean)} -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1`)
    return `Launched ${clean}`
  }

  return {
    daemonPid, activeProfile, cpuLittle, cpuBig, cpuGov, cpuCores, gpuInfo,
    sysLoad, ramUsage, ramPercent, zramUsage, zramPercent, zramConfiguredMb, zramLiveMb, zramComp, ioInfo, vmInfo,
    cpuTemp, batTemp, gpuTemp, batStatus, batLevel,
    games, logs, loading,
    moduleVersion, kernelVersion, chipset, uptime,
    isRunning,
    refresh, flushRam, setZramSize, rebootDevice, restartDaemon, exportLogs, clearLogs, createShortcut,
    addGame, removeGame, updateGameProfile, autoDetectGames, launchGame, setLogsActive, stopUptimeTicker,
    startCardPolling, stopCardPolling
  }
})
