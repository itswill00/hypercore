<template>
  <div class="logs-page-container">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">Activity Log</div>
        <div class="page-header-sub">History of profile switches and system events</div>
      </div>
      <div style="display: flex; gap: 6px; align-items: center;">
        <button class="btn-md3 btn-md3-secondary btn-md3-sm" title="Reset terms disclaimer" @click="resetDisclaimer">
          Terms
        </button>
        <button class="btn-md3 btn-md3-secondary btn-md3-sm" title="Copy all logs" @click="copyLog">
          Copy
        </button>
        <button class="btn-md3 btn-md3-secondary btn-md3-sm" title="Export Bugreport ZIP" @click="saveLog">
          Export
        </button>
        <button class="btn-md3 btn-md3-danger btn-md3-sm" title="Clear log content" @click="clearLog">
          Clear
        </button>
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
  </div>
</template>

<script setup>
import { ref, computed, inject, watch, nextTick, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
const toast = inject('toast')

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

async function saveLog() {
  const msg = await store.exportLogs()
  if (toast) toast(msg)
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
  font-weight: 600;
  letter-spacing: 0.3px;
  text-transform: uppercase;
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

.lbl-info { background: rgba(160, 178, 198, 0.15); color: #a0b2c6; }
.lbl-state { background: rgba(226, 229, 236, 0.15); color: #e2e5ec; }
.lbl-warn { background: rgba(210, 200, 176, 0.15); color: #d2c8b0; }
.lbl-error { background: rgba(229, 115, 115, 0.15); color: #e57373; }

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

.txt-gaming {
  color: #a0b2c6;
  font-weight: 600;
}

.txt-thermal {
  color: #d2c8b0;
}

.txt-start {
  color: #e2e5ec;
  font-weight: 600;
}

.log-line-enter-active {
  transition: opacity 0.2s ease, transform 0.2s ease;
}

.log-line-enter-from {
  opacity: 0;
  transform: translateY(6px);
}
</style>
