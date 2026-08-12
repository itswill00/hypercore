<template>
  <div class="logs-page-container" @click="closeMenu">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">Activity Log</div>
        <div class="page-header-sub">History of profile switches and system events</div>
      </div>
      <div class="menu-relative-container" @click.stop>
        <button
          class="btn-icon-md3"
          title="Log options"
          @click="toggleMenu"
          :class="{ 'active': menuOpen }"
        >
          <Icons name="more-vertical" :size="18" />
        </button>

        <Transition name="menu-pop">
          <div v-if="menuOpen" class="dropdown-menu-md3">
            <button class="menu-item" @click="handleAction(copyLog)">
              <span>Copy All Logs</span>
            </button>
            <button class="menu-item" @click="handleAction(saveLog)">
              <span>Export Bugreport</span>
            </button>
            <button class="menu-item" @click="handleAction(resetDisclaimer)">
              <span>Terms Disclaimer</span>
            </button>
            <div class="menu-divider"></div>
            <button class="menu-item menu-item-danger" @click="handleAction(clearLog)">
              <span>Clear Log</span>
            </button>
          </div>
        </Transition>
      </div>
    </div>

    <div class="console-sub-bar">
      <span class="sub-bar-title">Console output</span>
      <span class="sub-bar-count">
        {{ parsedLogs.length }} entries
      </span>
    </div>

    <div
      ref="logContainer"
      class="terminal-viewport"
      @scroll="handleScroll"
    >
      <TransitionGroup name="log-line" tag="div" class="terminal-content">
        <div
          v-for="(item, idx) in parsedLogs"
          :key="idx"
          class="terminal-line"
          :style="{ '--i': Math.min(idx, 8) }"
        >
          <span class="t-stamp" v-if="item.stamp">{{ item.stamp }}</span>
          <span v-if="item.level" class="t-level" :class="item.levelClass">{{ item.level }}</span>
          <span v-if="item.tag" class="t-tag">{{ item.tag }}</span>
          <span class="t-text" :class="item.textClass">{{ item.msg }}</span>
        </div>
      </TransitionGroup>
    </div>

    <BugreportModal :show="showBugreport" @close="showBugreport = false" />
  </div>
</template>

<script setup>
import { ref, computed, inject, watch, nextTick, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'
import BugreportModal from '@/components/BugreportModal.vue'

const store = useHyperStore()
const toast = inject('toast')

const menuOpen = ref(false)
const showBugreport = ref(false)

function toggleMenu() {
  menuOpen.value = !menuOpen.value
}

function closeMenu() {
  menuOpen.value = false
}

function handleAction(fn) {
  closeMenu()
  fn()
}

const logContainer = ref(null)
const autoScroll = ref(true)

const parsedLogs = computed(() => {
  if (!store.logs) return []
  const rawLines = store.logs.split('\n').filter(l => l.trim().length > 0)

  return rawLines.map(line => {
    const m = line.match(/^(?:\[?(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d{3})?)\]?)\s*(?:\[([^\]]+)\])?\s*(?:\[([^\]]+)\])?\s*(.*)$/)
    if (m && m[1]) {
      let stamp = m[1] || ''
      if (stamp.includes(' ')) {
        const timePart = stamp.split(' ')[1] || stamp
        stamp = timePart.split('.')[0]
      }
      const level = (m[2] || '').trim()
      const tag = (m[3] || '').trim()
      const msg = m[4] || line

      let levelClass = 'lbl-info'
      if (level === 'STATE') levelClass = 'lbl-state'
      else if (level === 'WARN') levelClass = 'lbl-warn'
      else if (level === 'ERROR') levelClass = 'lbl-error'

      let textClass = ''
      if (msg.includes('Gaming') || tag === 'GAME') textClass = 'txt-gaming'
      else if (msg.includes('Thermal Tier') || level === 'WARN') textClass = 'txt-thermal'
      else if (msg.includes('started')) textClass = 'txt-start'

      return { stamp, level, tag, msg, levelClass, textClass }
    }

    return { stamp: '', level: '', tag: '', msg: line, levelClass: '', textClass: '' }
  })
})

function handleScroll() {
  if (!logContainer.value) return
  const el = logContainer.value
  const isAtBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 40
  autoScroll.value = isAtBottom
}

function scrollToBottom() {
  nextTick(() => {
    if (logContainer.value && autoScroll.value) {
      logContainer.value.scrollTop = logContainer.value.scrollHeight
    }
  })
}

async function copyLog() {
  try {
    const text = store.logs || ''
    if (navigator.clipboard && navigator.clipboard.writeText) {
      await navigator.clipboard.writeText(text)
    } else {
      const textarea = document.createElement('textarea')
      textarea.value = text
      document.body.appendChild(textarea)
      textarea.select()
      document.execCommand('copy')
      document.body.removeChild(textarea)
    }
    if (toast) toast('Log copied')
  } catch {
    if (toast) toast('Failed to copy')
  }
}

function saveLog() {
  showBugreport.value = true
}

async function clearLog() {
  const msg = await store.clearLogs()
  if (toast) toast(msg)
}

function resetDisclaimer() {
  localStorage.removeItem('hypercore_disclaimer_agreed')
  if (window.resetDisclaimer) {
    window.resetDisclaimer()
  } else {
    window.dispatchEvent(new CustomEvent('reset-disclaimer'))
  }
}

watch(() => store.logs, () => {
  scrollToBottom()
})

onMounted(() => {
  store.setLogsActive(true)
  store.refresh()
  if (logContainer.value) {
    logContainer.value.scrollTop = logContainer.value.scrollHeight
  }
})

onUnmounted(() => {
  store.setLogsActive(false)
})
</script>

<style scoped>
.logs-page-container {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #08080a;
}

.console-sub-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 16px;
  background: #0d0e12;
  border-bottom: 1px solid var(--surface-container-high);
}

.sub-bar-title {
  font-size: 11px;
  color: var(--on-surface-variant);
  font-weight: 500;
  letter-spacing: 0.2px;
}

.sub-bar-count {
  font-size: 11px;
  font-family: var(--font-mono);
  color: var(--on-surface-variant);
  opacity: 0.6;
}

.terminal-viewport {
  flex: 1;
  background: #070709;
  font-family: var(--font-mono);
  font-size: 11px;
  line-height: 1.5;
  overflow-y: auto;
  overflow-x: auto;
  box-sizing: border-box;
  padding: 8px 12px calc(72px + var(--window-inset-bottom, 0px)) 12px;
  scrollbar-width: thin;
  scrollbar-color: var(--surface-container-highest) transparent;
}

.terminal-content {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: max-content;
}

.terminal-line {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 3px 6px;
  border-radius: 4px;
  line-height: 1.45;
  transition: background 0.12s ease;
  white-space: nowrap;
  box-sizing: border-box;
}

.terminal-line:hover {
  background: rgba(255, 255, 255, 0.05);
}

.t-stamp {
  color: var(--on-surface-variant);
  opacity: 0.45;
  font-size: 10px;
  flex-shrink: 0;
  padding-top: 1px;
  user-select: none;
}

.t-level {
  font-size: 9px;
  font-weight: 700;
  padding: 1px 5px;
  border-radius: 4px;
  flex-shrink: 0;
  user-select: none;
  letter-spacing: 0.5px;
  margin-top: 1px;
}

.lbl-info,
.lbl-state,
.lbl-warn,
.lbl-error {
  background: rgba(255, 255, 255, 0.08);
  color: #f0f2f5;
}

.t-tag {
  font-size: 9px;
  font-weight: 600;
  color: var(--on-surface-variant);
  opacity: 0.85;
  padding: 1px 5px;
  background: var(--surface-container-high);
  border-radius: 4px;
  flex-shrink: 0;
  margin-top: 1px;
}

.t-text {
  color: var(--on-surface);
  white-space: nowrap;
  word-break: normal;
}

.txt-gaming,
.txt-thermal,
.txt-start {
  color: var(--on-surface);
  font-weight: 600;
}

.menu-relative-container {
  position: relative;
}

.btn-icon-md3 {
  width: 32px;
  height: 32px;
  border-radius: 16px;
  background: var(--surface-container-high);
  border: 1px solid rgba(255, 255, 255, 0.08);
  color: var(--on-surface);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.15s ease;
}

.btn-icon-md3:active,
.btn-icon-md3.active {
  background: var(--surface-container-highest);
  border-color: var(--primary);
  color: var(--primary);
  transform: scale(0.94);
}

.dropdown-menu-md3 {
  position: absolute;
  top: calc(100% + 6px);
  right: 0;
  width: 170px;
  background: #16181e;
  border: 1px solid var(--surface-container-highest);
  border-radius: 12px;
  padding: 6px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.5);
  z-index: 100;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.menu-item {
  display: flex;
  align-items: center;
  width: 100%;
  padding: 8px 12px;
  border: none;
  background: transparent;
  color: var(--on-surface);
  font-size: 12px;
  font-weight: 500;
  border-radius: 8px;
  cursor: pointer;
  text-align: left;
  transition: background 0.12s ease;
}

.menu-item:hover,
.menu-item:active {
  background: rgba(255, 255, 255, 0.07);
}

.menu-item-danger {
  color: #f87171;
}

.menu-item-danger:hover,
.menu-item-danger:active {
  background: rgba(239, 68, 68, 0.15);
}

.menu-divider {
  height: 1px;
  background: var(--surface-container-high);
  margin: 4px 6px;
}

.menu-pop-enter-active,
.menu-pop-leave-active {
  transition: opacity 0.15s ease, transform 0.15s cubic-bezier(0.2, 0, 0, 1);
}

.menu-pop-enter-from,
.menu-pop-leave-to {
  opacity: 0;
  transform: scale(0.92) translateY(-6px);
}

.log-line-enter-active {
  transition: opacity 0.2s ease, transform 0.2s ease;
}

.log-line-enter-from {
  opacity: 0;
  transform: translateY(6px);
}
</style>
