<template>
  <div class="app-shell">
    <template v-if="isHudRoute">
      <div style="flex:1; overflow:hidden; position:relative;">
        <router-view />
      </div>
    </template>
    <template v-else>
      <div class="page-header page-header-outer" style="flex-shrink:0; transition: opacity 140ms ease;">
        <div :key="headerPath" style="display:flex; flex-direction:column;">
          <div class="page-header-title">{{ headerTitle }}</div>
          <div class="page-header-sub">{{ headerSub }}</div>
        </div>
        <span v-if="headerPath === '/'" class="badge-pill" style="font-size:11px; padding:4px 10px;">{{ store.moduleVersion || '—' }}</span>
        <span v-else-if="headerPath === '/charger'" class="badge-pill" style="font-size:11px; padding:4px 10px;" :style="!store.chargerSupported ? 'background:var(--surface-container-highest);color:var(--on-surface-variant);' : ''">{{ chargerHeaderBadge }}</span>
        <span v-else-if="headerPath === '/games'" class="badge-pill" style="font-size:11px; padding:4px 10px;">{{ store.games.length }} games</span>
        <span v-else-if="headerPath === '/about'" class="badge-pill" style="font-size:11px; padding:4px 10px;">{{ store.moduleVersion || '—' }}</span>
        <span v-else-if="headerPath === '/logs'" class="menu-relative-container" style="margin-left:auto;" @click.stop>
          <button class="btn-icon-md3" :class="{active: logsMenuOpen}" @click="logsMenuOpen=!logsMenuOpen" title="Log options">
            <Icons name="more-vertical" :size="18" />
          </button>
          <Transition name="menu-pop">
            <div v-if="logsMenuOpen" class="dropdown-menu-md3" style="position:absolute; top:calc(100%+6px); right:0;">
              <button class="menu-item" @click="onLogsAction('copy')">Copy All Logs</button>
              <button class="menu-item" @click="onLogsAction('export')">Export Bugreport</button>
              <button class="menu-item" @click="onLogsAction('disclaimer')">Terms Disclaimer</button>
              <div class="menu-divider"></div>
              <button class="menu-item menu-item-danger" @click="onLogsAction('clear')">Clear Log</button>
            </div>
          </Transition>
        </span>
      </div>
      <div ref="snapContainer" class="snap-container" :class="{ 'no-snap': snapSuspend }" style="transition: opacity 120ms ease;" @touchstart="onLogsTouchStart" @touchend="onLogsTouchEnd" @scroll.passive="onSnapScroll" @click="logsMenuOpen=false">
        <div class="tab-pane"><HomeView /></div>
        <div class="tab-pane"><ChargerView /></div>
        <div class="tab-pane"><GamesView /></div>
        <div class="tab-pane"><AboutView /></div>
        <div class="tab-pane"><LogsView /></div>
      </div>
    </template>
    <Navigation v-if="!isHudRoute" :active-path="activePath" @navigate="switchTab" />
    <DisclaimerModal />
    <BugreportModal :show="showLogsBugreport" @close="showLogsBugreport=false" />
    <transition name="toast-slide">
      <div v-if="toastMsg" class="toast-pill">
        <Icons name="check" :size="14" style="color: var(--primary);" />
        <span>{{ toastMsg }}</span>
      </div>
    </transition>
  </div>
</template>

<script setup>
import { ref, computed, provide, watch, onMounted, onUnmounted, nextTick } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useHyperStore } from '@/stores/hyper'
import Navigation from '@/components/Navigation.vue'
import DisclaimerModal from '@/components/DisclaimerModal.vue'
import Icons from '@/components/icons/Icons.vue'
import HomeView from '@/views/HomeView.vue'
import ChargerView from '@/views/ChargerView.vue'
import GamesView from '@/views/GamesView.vue'
import LogsView from '@/views/LogsView.vue'
import AboutView from '@/views/AboutView.vue'
import BugreportModal from '@/components/BugreportModal.vue'

const store = useHyperStore()
const route = useRoute()
const router = useRouter()

const toastMsg = ref('')
let toastTimer = null
let refreshInterval = null

const routesOrder = ['/', '/charger', '/games', '/about', '/logs']
const snapContainer = ref(null)
let isProgrammaticScroll = false
let scrollDebounceTimer = null
let scrollFromSnap = false
let rafId = 0
let scrollEndTimer = null
const snapIdx = ref(routesOrder.indexOf(route.path) !== -1 ? routesOrder.indexOf(route.path) : 0)
const headerIdx = ref(snapIdx.value)

const logsMenuOpen = ref(false)
const showLogsBugreport = ref(false)
const snapSuspend = ref(false)
let snapResumeTimer = null

const isHudRoute = computed(() => route.path === '/hud')
const activePath = computed(() => routesOrder[snapIdx.value] || '/')
const headerPath = computed(() => routesOrder[headerIdx.value] || '/')

const headerMap = {
  '/': { title: 'Dashboard', sub: 'Performance & Status Overview' },
  '/charger': { title: 'Charger Control', sub: 'Hardware charging speed & battery protection' },
  '/games': { title: 'Detected Games', sub: 'Installed games for auto profile switching' },
  '/logs': { title: 'Activity Log', sub: 'History of profile switches and system events' },
  '/about': { title: 'About Device', sub: 'System & Community' },
}
const headerTitle = computed(() => headerMap[headerPath.value]?.title || 'HyperCore')
const headerSub = computed(() => headerMap[headerPath.value]?.sub || '')

const sliderSteps = [
  { limit: 15, label: 'Trickle Minimum', current: '~500 mA' },
  { limit: 14, label: 'Gentle Overnight', current: '~750 mA' },
  { limit: 13, label: 'Cool Steady', current: '~1,200 mA' },
  { limit: 12, label: 'Mild Steady', current: '~1,600 mA' },
  { limit: 11, label: 'Daily Efficient', current: '~2,000 mA' },
  { limit: 10, label: 'Balanced Sweetspot', current: '~2,300 mA' },
  { limit: 9, label: 'Balanced Boost', current: '~2,550 mA' },
  { limit: 8, label: 'Express Flow', current: '~2,800 mA' },
  { limit: 7, label: 'Express Boost', current: '~3,000 mA' },
  { limit: 6, label: 'High Speed', current: '~3,200 mA' },
  { limit: 5, label: 'Turbo Flow', current: '~3,450 mA' },
  { limit: 4, label: 'Turbo Rate', current: '~3,700 mA' },
  { limit: 3, label: 'Peak Flow', current: '~3,900 mA' },
  { limit: 2, label: 'Extreme Rate', current: '~4,100 mA' },
  { limit: 1, label: 'Ultra Peak', current: '~4,300 mA' },
  { limit: 0, label: 'Max Unrestricted', current: '~4,500 mA' },
]
const chargerHeaderBadge = computed(() => {
  if (!store.chargerSupported) return 'Unsupported'
  const f = sliderSteps.find(s => s.limit === store.customLimit)
  return f ? `${f.label} (${f.current})` : 'Balanced'
})

function switchTab(path, smooth = true) {
  const idx = routesOrder.indexOf(path)
  if (idx === -1) return
  if (idx === snapIdx.value && route.path === path) return
  logsMenuOpen.value = false
  if (snapResumeTimer) { clearTimeout(snapResumeTimer); snapResumeTimer = null }
  snapSuspend.value = false
  snapIdx.value = idx
  headerIdx.value = idx
  if (snapContainer.value) {
    isProgrammaticScroll = true
    snapContainer.value.scrollTo({ left: idx * snapContainer.value.clientWidth, behavior: smooth ? 'smooth' : 'auto' })
    if (scrollDebounceTimer) clearTimeout(scrollDebounceTimer)
    scrollDebounceTimer = setTimeout(() => { isProgrammaticScroll = false }, 420)
  }
  if (route.path !== path) router.replace(path)
}
async function onLogsAction(kind) {
  logsMenuOpen.value = false
  if (kind === 'copy') {
    try {
      const t = store.logs || ''
      if (navigator.clipboard?.writeText) await navigator.clipboard.writeText(t)
      else { const ta=document.createElement('textarea'); ta.value=t; document.body.appendChild(ta); ta.select(); document.execCommand('copy'); document.body.removeChild(ta) }
      toast('Log copied')
    } catch { toast('Failed to copy') }
  } else if (kind === 'export') {
    showLogsBugreport.value = true
  } else if (kind === 'disclaimer') {
    localStorage.removeItem('hypercore_disclaimer_agreed')
    if (window.resetDisclaimer) window.resetDisclaimer()
    else window.dispatchEvent(new CustomEvent('reset-disclaimer'))
  } else if (kind === 'clear') {
    const m = await store.clearLogs()
    if (m) toast(m)
  }
}
function onLogsTouchStart(e) {
  if (!e.target || !e.target.closest) return
  if (e.target.closest('.terminal-viewport')) {
    snapSuspend.value = true
    if (snapResumeTimer) clearTimeout(snapResumeTimer)
    snapResumeTimer = null
  }
}
function onLogsTouchEnd() {
  if (!snapSuspend.value) return
  if (isProgrammaticScroll) return
  if (snapResumeTimer) clearTimeout(snapResumeTimer)
  snapResumeTimer = setTimeout(() => { snapSuspend.value = false; snapResumeTimer = null }, 200)
}
function onSnapScroll() {
  if (isProgrammaticScroll) return
  if (!snapContainer.value) return
  if (rafId) return
  rafId = requestAnimationFrame(() => {
    rafId = 0
    if (!snapContainer.value || isProgrammaticScroll) return
    const w = snapContainer.value.clientWidth
    if (w === 0) return
    const raw = snapContainer.value.scrollLeft / w
    const liveIdx = Math.round(raw)
    const liveClamped = Math.max(0, Math.min(routesOrder.length - 1, liveIdx))
    const dist = Math.abs(raw - liveClamped)
    if (dist < 0.35 && liveClamped !== snapIdx.value) {
      snapIdx.value = liveClamped
    }
    if (scrollEndTimer) clearTimeout(scrollEndTimer)
    scrollEndTimer = setTimeout(() => {
      if (!snapContainer.value) return
      const finalIdx = Math.max(0, Math.min(routesOrder.length - 1, Math.round(snapContainer.value.scrollLeft / w)))
      const clamped = Math.max(0, Math.min(routesOrder.length - 1, finalIdx))
      if (clamped !== snapIdx.value) snapIdx.value = clamped
      if (clamped !== headerIdx.value) headerIdx.value = clamped
      const target = routesOrder[clamped]
      if (target && target !== route.path) {
        scrollFromSnap = true
        router.replace(target)
      }
    }, 110)
  })
}
function handleResize() {
  if (!snapContainer.value || isHudRoute.value) return
  const idx = snapIdx.value
  snapContainer.value.scrollTo({ left: idx * snapContainer.value.clientWidth, behavior: 'auto' })
}
watch(() => route.path, (to) => {
  if (isHudRoute.value) return
  if (scrollFromSnap) { scrollFromSnap = false; return }
  const idx = routesOrder.indexOf(to)
  if (idx === -1) return
  if (idx === snapIdx.value) return
  snapIdx.value = idx
  headerIdx.value = idx
  if (!snapContainer.value) return
  if (Math.abs(snapContainer.value.scrollLeft - idx * snapContainer.value.clientWidth) > 2) {
    isProgrammaticScroll = true
    snapContainer.value.scrollTo({ left: idx * snapContainer.value.clientWidth, behavior: 'smooth' })
    if (scrollDebounceTimer) clearTimeout(scrollDebounceTimer)
    scrollDebounceTimer = setTimeout(() => { isProgrammaticScroll = false }, 420)
  }
})
function toast(msg) { toastMsg.value = msg; if (toastTimer) clearTimeout(toastTimer); toastTimer = setTimeout(() => toastMsg.value='', 2500) }
provide('toast', toast)
function startPolling() { if (refreshInterval) return; store.refresh(); refreshInterval = setInterval(() => store.refresh(), 3000) }
function stopPolling() { if (refreshInterval) { clearInterval(refreshInterval); refreshInterval=null } }
function onVis() { if (document.hidden) { stopPolling(); store.stopUptimeTicker() } else startPolling() }
onMounted(() => {
  startPolling()
  document.addEventListener('visibilitychange', onVis)
  window.addEventListener('resize', handleResize)
  snapIdx.value = routesOrder.indexOf(route.path) !== -1 ? routesOrder.indexOf(route.path) : 0
  headerIdx.value = snapIdx.value
  nextTick(() => {
    handleResize()
    if (!isHudRoute.value && snapContainer.value) {
      const idx = snapIdx.value
      if (idx > 0) {
        isProgrammaticScroll = true
        snapContainer.value.scrollTo({ left: idx * snapContainer.value.clientWidth, behavior: 'auto' })
        setTimeout(() => isProgrammaticScroll=false, 60)
      }
    }
  })
})
onUnmounted(() => {
  stopPolling(); store.stopUptimeTicker();
  document.removeEventListener('visibilitychange', onVis);
  window.removeEventListener('resize', handleResize);
  if (scrollDebounceTimer) clearTimeout(scrollDebounceTimer)
  if (scrollEndTimer) clearTimeout(scrollEndTimer)
  if (snapResumeTimer) clearTimeout(snapResumeTimer)
  if (rafId) cancelAnimationFrame(rafId)
})
</script>

<style scoped>
.menu-relative-container { position: relative; }
.btn-icon-md3 {
  width: 32px; height: 32px; border-radius: 16px;
  background: var(--surface-container-high);
  border: 1px solid rgba(255,255,255,0.08);
  color: var(--on-surface);
  display: flex; align-items: center; justify-content: center;
  cursor: pointer; transition: all 0.15s ease;
}
.btn-icon-md3:active, .btn-icon-md3.active {
  background: var(--surface-container-highest);
  border-color: var(--primary);
  color: var(--primary);
  transform: scale(0.94);
}
.dropdown-menu-md3 {
  position: absolute; top: calc(100% + 6px); right: 0;
  width: 170px; background: #16181e;
  border: 1px solid var(--surface-container-highest);
  border-radius: 12px; padding: 6px;
  box-shadow: 0 8px 24px rgba(0,0,0,0.5);
  z-index: 100; display: flex; flex-direction: column; gap: 2px;
}
.menu-item {
  display: flex; align-items: center; width: 100%;
  padding: 8px 12px; border: none; background: transparent;
  color: var(--on-surface); font-size: 12px; font-weight: 500;
  border-radius: 8px; cursor: pointer; text-align: left;
  transition: background 0.12s ease;
}
.menu-item:hover, .menu-item:active { background: rgba(255,255,255,0.07); }
.menu-item-danger { color: #f87171; }
.menu-item-danger:hover, .menu-item-danger:active { background: rgba(239,68,68,0.15); }
.menu-divider { height: 1px; background: var(--surface-container-high); margin: 4px 6px; }
.menu-pop-enter-active { transition: opacity 0.18s ease, transform 0.22s cubic-bezier(0.34,1.56,0.64,1); transform-origin: top right; }
.menu-pop-leave-active { transition: opacity 0.14s ease, transform 0.14s cubic-bezier(0.4,0,0.2,1); transform-origin: top right; }
.menu-pop-enter-from { opacity: 0; transform: scale(0.88) translateY(-4px); }
.menu-pop-leave-to { opacity: 0; transform: scale(0.92) translateY(-4px); }

.toast-pill {
  position: fixed;
  bottom: calc(88px + var(--window-inset-bottom, 0px));
  left: 50%;
  transform: translateX(-50%);
  background: var(--surface-container-highest);
  border: 1px solid var(--surface-bright);
  color: var(--on-surface);
  font-size: 12px; font-weight: 500;
  padding: 8px 16px; border-radius: 20px;
  box-shadow: 0 4px 16px rgba(0,0,0,0.4);
  z-index: 99; pointer-events: none;
  display: flex; align-items: center; gap: 8px;
}
.toast-slide-enter-active { transition: transform 0.32s cubic-bezier(0.34,1.56,0.64,1), opacity 0.25s ease; }
.toast-slide-leave-active { transition: transform 0.22s cubic-bezier(0.4,0,0.2,1), opacity 0.18s ease; }
.toast-slide-enter-from { opacity:0; transform: translate(-50%,16px) scale(0.92); }
.toast-slide-leave-to { opacity:0; transform: translate(-50%,-8px) scale(0.96); }
</style>
