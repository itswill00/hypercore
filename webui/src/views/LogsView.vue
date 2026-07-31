<template>
  <div style="height: 100%; display: flex; flex-direction: column; gap: 8px;">
    <!-- Header -->
    <div class="page-header" style="margin-bottom: 0;">
      <div>
        <div class="page-header-title">System log</div>
        <div class="page-header-sub">HyperCore By @itswill00</div>
      </div>
      <div style="display: flex; gap: 8px; align-items: center;">
        <button
          class="btn-md3"
          :class="paused ? 'btn-md3-primary' : 'btn-md3-secondary'"
          style="padding: 6px 12px; font-size: 11px;"
          @click="togglePause"
        >
          {{ paused ? 'Resume' : 'Pause' }}
        </button>

        <button
          class="btn-md3 btn-md3-secondary"
          style="padding: 6px 12px; font-size: 11px;"
          title="Clear console view"
          @click="clearLogs"
        >
          Clear
        </button>

        <button
          class="btn-md3 btn-md3-secondary"
          style="padding: 6px 12px; font-size: 11px;"
          title="Copy log"
          @click="copyLog"
        >
          Copy
        </button>

        <button
          class="btn-md3 btn-md3-primary"
          style="padding: 6px 12px; font-size: 11px;"
          title="Export log"
          @click="saveLog"
        >
          Save
        </button>
      </div>
    </div>

    <!-- Search & Filter Controls -->
    <div style="display: flex; gap: 8px; align-items: center; flex-wrap: wrap;">
      <!-- Search Input -->
      <div class="search-box" style="flex: 1; min-width: 140px;">
        <svg viewBox="0 0 24 24" width="14" height="14" fill="currentColor" style="opacity: 0.6;">
          <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
        </svg>
        <input
          v-model="searchQuery"
          type="text"
          placeholder="Filter logs or tags..."
          class="search-input"
        />
        <button v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">✕</button>
      </div>

      <!-- Level Filter Chips -->
      <div style="display: flex; gap: 4px; align-items: center;">
        <button
          v-for="lvl in levelFilters"
          :key="lvl.id"
          class="chip-btn"
          :class="{ active: selectedLevel === lvl.id }"
          @click="selectedLevel = lvl.id"
        >
          {{ lvl.label }}
        </button>
      </div>
    </div>

    <!-- Entry Counter -->
    <div style="display: flex; align-items: center; justify-content: space-between; padding: 0 4px;">
      <span style="font-size: 11px; color: var(--on-surface-variant); opacity: 0.8;">
        Showing {{ filteredLogs.length }} of {{ parsedLogs.length }} entries
      </span>
      <span style="font-size: 10px; font-family: var(--font-mono); color: var(--on-surface-variant); opacity: 0.6;">
        {{ paused ? '● PAUSED' : '● LIVE STREAMING' }}
      </span>
    </div>

    <!-- Terminal Output Box -->
    <div
      ref="logContainer"
      class="terminal-box"
      @scroll="handleScroll"
    >
      <div v-if="filteredLogs.length === 0" class="empty-log">
        No log entries match the selected filter.
      </div>

      <div
        v-for="(item, idx) in filteredLogs"
        :key="idx"
        class="terminal-line"
      >
        <span class="t-stamp" v-if="item.stamp">{{ item.stamp }}</span>
        <span v-if="item.level" class="t-level" :class="item.levelClass">{{ item.level }}</span>
        <span v-if="item.tag" class="t-tag">{{ item.tag }}</span>
        <span class="t-text" :class="item.textClass">{{ item.msg }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, inject, watch, nextTick, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
const toast = inject('toast')

const logContainer = ref(null)
const autoScroll = ref(true)
const paused = ref(false)
const searchQuery = ref('')
const selectedLevel = ref('ALL')
const cleared = ref(false)

const levelFilters = [
  { id: 'ALL', label: 'All' },
  { id: 'STATE', label: 'State' },
  { id: 'WARN', label: 'Warn' },
  { id: 'ERROR', label: 'Error' },
  { id: 'INFO', label: 'Info' }
]

const parsedLogs = computed(() => {
  if (cleared.value || !store.logs) return []
  const rawLines = store.logs.split('\n').filter(l => l.trim().length > 0)

  return rawLines.map(line => {
    const m = line.match(/^(?:\[?(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d{3})?)\]?)\s*(?:\[([^\]]+)\])?\s*(?:\[([^\]]+)\])?\s*(.*)$/)
    if (m && m[1]) {
      const stamp = m[1] || ''
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

      return { stamp, level, tag, msg, levelClass, textClass, raw: line }
    }

    return { stamp: '', level: '', tag: '', msg: line, levelClass: '', textClass: '', raw: line }
  })
})

const filteredLogs = computed(() => {
  let list = parsedLogs.value

  if (selectedLevel.value !== 'ALL') {
    list = list.filter(item => item.level === selectedLevel.value)
  }

  if (searchQuery.value.trim()) {
    const q = searchQuery.value.toLowerCase()
    list = list.filter(item =>
      item.msg.toLowerCase().includes(q) ||
      item.tag.toLowerCase().includes(q) ||
      item.level.toLowerCase().includes(q)
    )
  }

  return list
})

function handleScroll() {
  if (!logContainer.value) return
  const el = logContainer.value
  const isAtBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 40
  autoScroll.value = isAtBottom
}

function scrollToBottom() {
  if (paused.value) return
  nextTick(() => {
    if (logContainer.value && autoScroll.value) {
      logContainer.value.scrollTop = logContainer.value.scrollHeight
    }
  })
}

function togglePause() {
  paused.value = !paused.value
  if (!paused.value) {
    scrollToBottom()
  }
}

function clearLogs() {
  cleared.value = true
  if (toast) toast('Console cleared')
}

async function copyLog() {
  try {
    const text = filteredLogs.value.map(i => i.raw).join('\n') || store.logs || ''
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
    if (toast) toast('Filtered log copied')
  } catch {
    if (toast) toast('Failed to copy')
  }
}

async function saveLog() {
  const msg = await store.exportLogs()
  if (toast) toast(msg)
}

watch(() => store.logs, () => {
  if (cleared.value) cleared.value = false
  scrollToBottom()
})

onMounted(() => {
  if (logContainer.value) {
    logContainer.value.scrollTop = logContainer.value.scrollHeight
  }
})
</script>

<style scoped>
.search-box {
  display: flex;
  align-items: center;
  gap: 6px;
  background: var(--surface-container-high);
  border-radius: 8px;
  padding: 4px 10px;
}

.search-input {
  border: none;
  background: transparent;
  color: var(--on-surface);
  font-size: 11px;
  width: 100%;
  outline: none;
}

.clear-btn {
  background: none;
  border: none;
  color: var(--on-surface-variant);
  font-size: 12px;
  cursor: pointer;
  padding: 0 4px;
}

.chip-btn {
  background: var(--surface-container-high);
  border: 1px solid transparent;
  color: var(--on-surface-variant);
  font-size: 10px;
  font-weight: 600;
  padding: 4px 8px;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.15s ease;
}

.chip-btn.active {
  background: rgba(186, 104, 200, 0.2);
  color: var(--md-sys-color-primary, #ba68c8);
  border-color: rgba(186, 104, 200, 0.4);
}

.terminal-box {
  flex: 1;
  background: #0f0e13;
  border: 1px solid var(--surface-container-high);
  border-radius: 12px;
  padding: 12px;
  font-family: var(--font-mono);
  font-size: 11px;
  line-height: 1.6;
  overflow-y: auto;
  box-sizing: border-box;
}

.empty-log {
  color: var(--on-surface-variant);
  opacity: 0.5;
  text-align: center;
  padding: 24px 0;
  font-size: 11px;
}

.terminal-line {
  display: flex;
  align-items: center;
  gap: 8px;
  word-break: break-all;
  white-space: pre-wrap;
  padding: 2px 0;
}

.t-stamp {
  color: var(--on-surface-variant);
  opacity: 0.45;
  font-size: 10px;
  flex-shrink: 0;
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
}

.lbl-info { background: rgba(100, 181, 246, 0.15); color: #64b5f6; }
.lbl-state { background: rgba(186, 104, 200, 0.15); color: #ba68c8; }
.lbl-warn { background: rgba(255, 183, 77, 0.15); color: #ffb74d; }
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
}

.t-text {
  color: var(--on-surface);
  flex: 1;
}

.txt-gaming {
  color: #81c784;
  font-weight: 600;
}

.txt-thermal {
  color: #ffb74d;
}

.txt-start {
  color: #64b5f6;
  font-weight: 600;
}
</style>
