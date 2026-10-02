import { defineStore } from 'pinia'
import { ref } from 'vue'
import { execCommand } from '@/helpers/shell'

export const useHyperMoonStore = defineStore('hypermoon', () => {
  const HUD_DIR = '/data/adb/hypercore/hud'
  const CFG_FILE = `${HUD_DIR}/config.json`
  const POS_FILE = `${HUD_DIR}/position.json`
  const STATS_FILE = `${HUD_DIR}/stats.json`

  const isRunning = ref(false)
  const overlayPid = ref('')
  const daemonPid = ref('')
  const loading = ref(false)

  const config = ref({
    visible: false,
    auto_gaming: false,
    show_fps: true,
    show_cpu: true,
    show_cpu_freq: true,
    show_gov: false,
    show_gpu: true,
    show_gpu_freq: true,
    show_gpu_gov: false,
    show_ram: true,
    show_zram: false,
    show_battery: true,
    show_net: true,
    is_horizontal: false,
    align: 'left',
    theme: 'cyber_neon',
    custom_color: '#6366F1',
    opacity: 0.70,
    scale: 0.65,
    font_size: 14,
    corner_radius: 14,
    bg_width: 150,
    bg_height: 160,
    refresh_interval: 1500,
    target_fps: 60
  })

  const stats = ref({
    fps: '--',
    frametime: '--',
    cpu_load: '--',
    cpu_freq: '--',
    cpu_temp: '--',
    gpu_load: '--',
    gpu_freq: '--',
    gpu_temp: '--',
    ram_used: '--',
    bat_watt: '--',
    bat_temp: '--',
    screen_hz: '--'
  })

  let debounceTimer = null
  let pollingTimer = null
  let posTimer = null

  const position = ref({ x: 697, y: 411 })

  async function loadPosition() {
    try {
      const out = await execCommand(`cat ${POS_FILE} 2>/dev/null`)
      if (out && out.trim().startsWith('{')) {
        const parsed = JSON.parse(out.trim())
        if (Number.isFinite(+parsed.x)) position.value.x = Math.max(0, Math.round(+parsed.x))
        if (Number.isFinite(+parsed.y)) position.value.y = Math.max(0, Math.round(+parsed.y))
      }
    } catch {}
  }

  function setPosition(x, y, immediate = false) {
    position.value.x = Math.max(0, Math.round(x))
    position.value.y = Math.max(0, Math.round(y))
    // The overlay (WindowManager AND hardware SurfaceControl paths) re-reads
    // position.json every refresh tick and clamps to screen bounds, so this
    // is the universal move primitive — including on Android 14 QPR3+/15
    // where direct finger-drag is impossible (no input channel on raw surfaces).
    if (posTimer) clearTimeout(posTimer)
    const write = () => {
      const json = JSON.stringify({ x: position.value.x, y: position.value.y })
      execCommand(`echo '${json}' > ${POS_FILE} && chmod 666 ${POS_FILE} 2>/dev/null`).catch(() => {})
    }
    if (immediate) write()
    else posTimer = setTimeout(write, 120)
  }

  function nudgePosition(dx, dy) {
    setPosition(position.value.x + dx, position.value.y + dy)
  }

  async function init() {
    await loadConfig()
    await loadPosition()
    await checkStatus()
    startPollingStats()
  }

  async function loadConfig() {
    try {
      const out = await execCommand(`cat ${CFG_FILE} 2>/dev/null`)
      if (out && out.trim().startsWith('{')) {
        const parsed = JSON.parse(out.trim())
        config.value = { ...config.value, ...parsed }
      }
    } catch {}
  }

  function saveConfigDebounced() {
    if (debounceTimer) clearTimeout(debounceTimer)
    debounceTimer = setTimeout(() => {
      saveConfig()
    }, 150)
  }

  async function saveConfig() {
    try {
      const jsonStr = JSON.stringify(config.value, null, 2)
      const b64 = btoa(unescape(encodeURIComponent(jsonStr)))
      await execCommand(`mkdir -p ${HUD_DIR} && echo "${b64}" | base64 -d > ${CFG_FILE} && chmod 644 ${CFG_FILE} 2>/dev/null`)
    } catch {}
  }

  async function checkStatus() {
    try {
      const opid = await execCommand("pgrep -f 'com.hypermoon.HyperMoonOverlay|com.fpsmoon.FPSMoonOverlay' 2>/dev/null | head -n1")
      // The binary is `hypermoon_d` (11 chars), not `hypermoon_daemon` (16).
      // pidof/pkill match the kernel's 15-char comm field, so the old name was
      // invisible to them: the liveness check below always reported "not
      // running" and every HUD toggle spawned another daemon. The -f form is
      // kept as a fallback so a future rename cannot silently reintroduce it.
      const dpid = await execCommand(
        "pidof hypermoon_d 2>/dev/null || pidof fpsmoon_d 2>/dev/null || " +
        "pgrep -f '/system/bin/hypermoon_d' 2>/dev/null | head -n1"
      )

      overlayPid.value = opid ? opid.trim() : ''
      daemonPid.value = dpid ? dpid.trim() : ''
      isRunning.value = Boolean(overlayPid.value || daemonPid.value)
    } catch {
      isRunning.value = false
    }
  }

  async function toggleMaster(enable) {
    // 1. Instant local state update (0ms UI latency)
    config.value.visible = enable
    isRunning.value = enable

    // 2. Immediate config write (bypasses debounce for instant kernel inotify)
    saveConfig().catch(() => {})

    // 3. Ensure engine background processes are alive when enabling
    if (enable) {
      const ensureCmd = `
        mkdir -p ${HUD_DIR} 2>/dev/null
        if ! pidof hypermoon_d >/dev/null 2>&1 && ! pgrep -f '/system/bin/hypermoon_d' >/dev/null 2>&1; then
          export HYPERMOON_STATE_DIR="${HUD_DIR}"
          nohup /data/adb/modules/hypercore/system/bin/hypermoon_d > "${HUD_DIR}/daemon.log" 2>&1 &
        fi
        if ! pgrep -f 'com.hypermoon.HyperMoonOverlay' >/dev/null 2>&1; then
          export HYPERMOON_STATE_DIR="${HUD_DIR}"
          CLASSPATH="/data/adb/modules/hypercore/system/bin/hypermoon.dex" nohup /system/bin/app_process /system/bin com.hypermoon.HyperMoonOverlay "${HUD_DIR}" > "${HUD_DIR}/overlay.log" 2>&1 &
        fi
      `
      execCommand(ensureCmd).then(() => {
        setTimeout(checkStatus, 300)
      }).catch(() => {})
    } else {
      setTimeout(checkStatus, 300)
    }

    return enable ? 'Overlay enabled' : 'Overlay disabled'
  }

  async function resetPosition() {
    try {
      setPosition(697, 411, true)
      return 'Position reset'
    } catch {
      return 'Failed to reset position'
    }
  }

  async function restartEngine() {
    loading.value = true
    try {
      await toggleMaster(false)
      // Actually stop the processes. Toggling visible off only writes config —
      // without this the "restart" just re-ran the liveness check against the
      // still-running daemon and reported success without restarting anything.
      await execCommand(
        "pkill -f 'com.hypermoon.HyperMoonOverlay' 2>/dev/null || true; " +
        "pkill -9 -x hypermoon_d 2>/dev/null || true; " +
        "pkill -9 -f '/system/bin/hypermoon_d' 2>/dev/null || true"
      ).catch(() => {})
      await new Promise(r => setTimeout(r, 600))
      await toggleMaster(true)
      return 'Overlay restarted'
    } catch {
      return 'Failed to restart overlay'
    } finally {
      loading.value = false
    }
  }

  function applyPreset(type) {
    const isHoriz = config.value.is_horizontal !== false
    if (type === 'compact') {
      config.value.show_fps = true
      config.value.show_cpu = true
      config.value.show_cpu_freq = true
      config.value.show_gov = false
      config.value.show_gpu = true
      config.value.show_gpu_freq = true
      config.value.show_gpu_gov = false
      config.value.show_ram = false
      config.value.show_zram = false
      config.value.show_battery = true
      config.value.show_net = false
      if (isHoriz) {
        config.value.bg_width = 250
        config.value.bg_height = 56
      } else {
        config.value.bg_width = 150
        config.value.bg_height = 120
      }
    } else if (type === 'minimal') {
      config.value.show_fps = true
      config.value.show_cpu = false
      config.value.show_cpu_freq = false
      config.value.show_gov = false
      config.value.show_gpu = false
      config.value.show_gpu_freq = false
      config.value.show_gpu_gov = false
      config.value.show_ram = false
      config.value.show_zram = false
      config.value.show_battery = false
      config.value.show_net = false
      if (isHoriz) {
        config.value.bg_width = 180
        config.value.bg_height = 42
      } else {
        config.value.bg_width = 130
        config.value.bg_height = 70
      }
    } else if (type === 'detailed') {
      config.value.show_fps = true
      config.value.show_cpu = true
      config.value.show_cpu_freq = true
      config.value.show_gov = true
      config.value.show_gpu = true
      config.value.show_gpu_freq = true
      config.value.show_gpu_gov = true
      config.value.show_ram = true
      config.value.show_zram = false
      config.value.show_battery = true
      config.value.show_net = true
      if (isHoriz) {
        config.value.bg_width = 340
        config.value.bg_height = 68
      } else {
        config.value.bg_width = 160
        config.value.bg_height = 220
      }
    }
    saveConfig()
  }

  function setLayout(isHorizontal) {
    config.value.is_horizontal = isHorizontal
    if (isHorizontal && config.value.bg_height > 70) {
      config.value.bg_width = 260
      config.value.bg_height = 56
    } else if (!isHorizontal && config.value.bg_width > 200) {
      config.value.bg_width = 150
      config.value.bg_height = 160
    }
    saveConfig()
  }

  function setAlignment(alignType) {
    config.value.align = alignType
    saveConfig()
  }

  function setTheme(themeName) {
    config.value.theme = themeName
    saveConfig()
  }

  let _pollInFlight = false
  function startPollingStats() {
    if (pollingTimer) clearInterval(pollingTimer)
    pollingTimer = setInterval(async () => {
      if (_pollInFlight) return
      _pollInFlight = true
      try {
        const out = await execCommand(`cat ${STATS_FILE} 2>/dev/null`)
        if (out && out.trim().startsWith('{')) {
          const parsed = JSON.parse(out.trim())
          stats.value = { ...stats.value, ...parsed }
        }
      } catch {} finally { _pollInFlight = false }
    }, 1500)
  }

  function stopPollingStats() {
    if (pollingTimer) {
      clearInterval(pollingTimer)
      pollingTimer = null
    }
  }

  return {
    config,
    stats,
    position,
    isRunning,
    overlayPid,
    daemonPid,
    loading,
    init,
    loadConfig,
    loadPosition,
    setPosition,
    nudgePosition,
    saveConfig,
    saveConfigDebounced,
    checkStatus,
    toggleMaster,
    resetPosition,
    restartEngine,
    applyPreset,
    setLayout,
    setAlignment,
    setTheme,
    startPollingStats,
    stopPollingStats
  }
})
