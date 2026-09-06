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
    show_net: false,
    is_horizontal: true,
    align: 'left',
    theme: 'cyber_neon',
    custom_color: '#6366F1',
    opacity: 0.60,
    scale: 0.80,
    font_size: 11,
    corner_radius: 14,
    bg_width: 150,
    bg_height: 160,
    refresh_interval: 850,
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

  async function init() {
    await loadConfig()
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
    }, 250)
  }

  async function saveConfig() {
    try {
      const jsonStr = JSON.stringify(config.value, null, 2)
      const b64 = btoa(unescape(encodeURIComponent(jsonStr)))
      await execCommand(`mkdir -p ${HUD_DIR} && echo "${b64}" | base64 -d > ${CFG_FILE} && chmod 666 ${CFG_FILE} 2>/dev/null`)
    } catch {}
  }

  async function checkStatus() {
    try {
      const opid = await execCommand("pgrep -f 'com.hypermoon.HyperMoonOverlay|com.fpsmoon.FPSMoonOverlay' 2>/dev/null | head -n1")
      const dpid = await execCommand("pidof hypermoon_daemon 2>/dev/null || pidof fpsmoon_daemon 2>/dev/null")

      overlayPid.value = opid ? opid.trim() : ''
      daemonPid.value = dpid ? dpid.trim() : ''
      isRunning.value = Boolean(overlayPid.value || daemonPid.value)
    } catch {
      isRunning.value = false
    }
  }

  async function toggleMaster(enable) {
    loading.value = true
    config.value.visible = enable
    await saveConfig()

    try {
      if (enable) {
        const grantCmd = [
          'mkdir -p /data/adb/hypercore/hud 2>/dev/null',
          'chmod 777 /data/adb/hypercore/hud 2>/dev/null',
          'cmd appops set --uid 0 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true',
          'cmd appops set --uid 1000 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true',
          'cmd appops set --uid 2000 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true',
          'cmd appops set android SYSTEM_ALERT_WINDOW allow 2>/dev/null || true',
          'cmd appops set com.android.shell SYSTEM_ALERT_WINDOW allow 2>/dev/null || true',
          'pm grant com.android.shell android.permission.SYSTEM_ALERT_WINDOW 2>/dev/null || true'
        ].join('; ')
        await execCommand(grantCmd)

        const startDaemon = `
          if ! pidof hypermoon_daemon >/dev/null 2>&1; then
            export HYPERMOON_STATE_DIR="${HUD_DIR}"
            nohup /data/adb/modules/hypercore/system/bin/hypermoon_daemon > "${HUD_DIR}/daemon.log" 2>&1 &
          fi
        `
        await execCommand(startDaemon)

        const startOverlay = `
          if ! pgrep -f 'com.hypermoon.HyperMoonOverlay' >/dev/null 2>&1; then
            export HYPERMOON_STATE_DIR="${HUD_DIR}"
            CLASSPATH="/data/adb/modules/hypercore/system/bin/hypermoon.dex" nohup /system/bin/app_process /system/bin com.hypermoon.HyperMoonOverlay "${HUD_DIR}" > "${HUD_DIR}/overlay.log" 2>&1 &
          fi
        `
        await execCommand(startOverlay)
      } else {
        const killCmd = `
          for p in $(pgrep -f '[H]yperMoonOverlay|[F]PSMoonOverlay' 2>/dev/null); do [ "$p" != "$$" ] && kill -9 "$p" 2>/dev/null || true; done
          pkill -9 -x hypermoon_daemon 2>/dev/null || true
          pkill -9 -x fpsmoon_daemon 2>/dev/null || true
        `
        await execCommand(killCmd)
      }
      await new Promise(r => setTimeout(r, 400))
      await checkStatus()
      return enable ? 'HyperMoon HUD activated' : 'HyperMoon HUD stopped'
    } catch {
      return 'Failed to toggle HyperMoon HUD'
    } finally {
      loading.value = false
    }
  }

  async function resetPosition() {
    try {
      const defPos = JSON.stringify({ x: 48, y: 96 }, null, 2)
      await execCommand(`echo '${defPos}' > ${POS_FILE} && chmod 666 ${POS_FILE} 2>/dev/null`)
      return 'Overlay position reset to default'
    } catch {
      return 'Failed to reset position'
    }
  }

  async function restartEngine() {
    loading.value = true
    try {
      await toggleMaster(false)
      await new Promise(r => setTimeout(r, 300))
      await toggleMaster(true)
      return 'HyperMoon engine restarted'
    } catch {
      return 'Failed to restart HyperMoon engine'
    } finally {
      loading.value = false
    }
  }

  function startPollingStats() {
    if (pollingTimer) clearInterval(pollingTimer)
    pollingTimer = setInterval(async () => {
      try {
        const out = await execCommand(`cat ${STATS_FILE} 2>/dev/null`)
        if (out && out.trim().startsWith('{')) {
          const parsed = JSON.parse(out.trim())
          stats.value = { ...stats.value, ...parsed }
        }
      } catch {}
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
    isRunning,
    overlayPid,
    daemonPid,
    loading,
    init,
    loadConfig,
    saveConfig,
    saveConfigDebounced,
    checkStatus,
    toggleMaster,
    resetPosition,
    restartEngine,
    startPollingStats,
    stopPollingStats
  }
})
