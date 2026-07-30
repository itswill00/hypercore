<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Sticky Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">System log</div>
        <div class="page-header-sub">HyperCore By @itswill00</div>
      </div>
      <div style="display: flex; gap: 6px; align-items: center;">
        <button class="btn-md3 btn-md3-secondary btn-icon-only" title="Refresh log" @click="store.refresh()">
          <Icons name="refresh" :size="16" />
        </button>
        <button class="btn-md3 btn-md3-secondary btn-icon-only" title="Copy log text" @click="copyLog">
          <Icons name="check" :size="15" />
        </button>
        <button class="btn-md3 btn-md3-primary" style="padding: 6px 12px;" title="Export to SDCard" @click="saveLog">
          <Icons name="logs" :size="14" />
          <span>Save</span>
        </button>
      </div>
    </div>

    <!-- Scroll Content -->
    <div class="content-area" style="display: flex; flex-direction: column; gap: 12px; height: 100%;">
      <!-- Control Bar: Search & Quick Filters -->
      <div class="md3-card" style="padding: 12px; margin-bottom: 0;">
        <div style="display: flex; gap: 8px; margin-bottom: 10px; align-items: center;">
          <div style="position: relative; flex: 1;">
            <input
              v-model="searchQuery"
              type="text"
              placeholder="Search logs..."
              class="log-search-input"
            />
            <span v-if="searchQuery" class="clear-search" @click="searchQuery = ''">✕</span>
          </div>
          <button
            class="btn-md3"
            :class="autoScroll ? 'btn-md3-primary' : 'btn-md3-secondary'"
            style="padding: 6px 10px; font-size: 11px;"
            @click="toggleAutoScroll"
          >
            {{ autoScroll ? 'Auto-scroll ON' : 'Paused' }}
          </button>
        </div>

        <!-- Filter Chips -->
        <div style="display: flex; gap: 6px; overflow-x: auto; padding-bottom: 2px;">
          <button
            v-for="f in filters"
            :key="f.id"
            class="filter-chip"
            :class="{ 'active': activeFilter === f.id }"
            @click="activeFilter = f.id"
          >
            {{ f.label }}
          </button>
        </div>
      </div>

      <!-- Log Output Console -->
      <div class="md3-card" style="flex: 1; display: flex; flex-direction: column; padding: 12px; margin-bottom: 0; min-height: 0;">
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; flex-shrink: 0;">
          <div style="display: flex; align-items: center; gap: 6px;">
            <span class="live-dot"></span>
            <span style="font-size: 12px; font-weight: 600; color: var(--on-surface);">Live console</span>
          </div>
          <div style="display: flex; gap: 8px; align-items: center;">
            <span style="font-size: 10px; font-family: var(--font-mono); color: var(--on-surface-variant);">{{ filteredLines.length }} lines</span>
            <span style="font-size: 10px; font-family: var(--font-mono); color: var(--on-surface-variant); opacity: 0.6;">/sdcard/Android/hypercore.log</span>
          </div>
        </div>

        <div
          ref="logContainer"
          class="log-box"
          @scroll="handleUserScroll"
        >
          <div v-if="filteredLines.length === 0" style="padding: 30px 0; text-align: center; color: var(--on-surface-variant); font-size: 12px;">
            No log entries match your filter
          </div>
          <div
            v-for="(line, idx) in filteredLines"
            :key="idx"
            class="log-line"
            :class="lineClass(line)"
          >
            <span class="log-time" v-if="getTimestamp(line)">{{ getTimestamp(line) }}</span>
            <span class="log-body" v-html="formatLineBody(line)"></span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, inject, watch, nextTick, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const searchQuery = ref('')
const activeFilter = ref('all')
const autoScroll = ref(true)
const logContainer = ref(null)
let isUserScrolling = false

const filters = [
  { id: 'all', label: 'All' },
  { id: 'profile', label: 'Profiles 🔄' },
  { id: 'gaming', label: 'Gaming ⚡' },
  { id: 'thermal', label: 'Thermal 🌡️' }
]

const rawLines = computed(() => {
  if (!store.logs) return []
  return store.logs.split('\n').filter(l => l.trim().length > 0)
})

const filteredLines = computed(() => {
  return rawLines.value.filter(line => {
    // 1. Text Search Filter
    if (searchQuery.value) {
      const q = searchQuery.value.toLowerCase()
      if (!line.toLowerCase().includes(q)) return false
    }

    // 2. Chip Category Filter
    if (activeFilter.value === 'profile') {
      return line.includes('Profile:') || line.includes('Touch') || line.includes('Interactive') || line.includes('LOCK')
    }
    if (activeFilter.value === 'gaming') {
      return line.includes('Gaming') || line.includes('Boost') || line.includes('Game')
    }
    if (activeFilter.value === 'thermal') {
      return line.includes('Thermal Tier') || line.includes('T3') || line.includes('T2') || line.includes('CPU:')
    }

    return true
  })
})

function getTimestamp(line) {
  const m = line.match(/^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})/)
  return m ? m[1] : ''
}

function formatLineBody(line) {
  let body = line.replace(/^\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\s*/, '')

  // Escape HTML characters
  body = body.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')

  // Highlight profiles
  body = body.replace(/(Profile:\s*)([^\s\(]+)/g, '$1<span class="hl-profile">$2</span>')
  body = body.replace(/(Interactive|Gaming|Touch|Saver|Sleep|AUTO)/gi, '<span class="hl-mode">$1</span>')

  // Highlight thermal tiers
  body = body.replace(/(Thermal Tier:\s*)([^\n]+)/g, '$1<span class="hl-thermal">$2</span>')

  // Highlight temperatures
  body = body.replace(/(CPU:\s*\d+°C)/gi, '<span class="hl-temp-cpu">$1</span>')
  body = body.replace(/(Bat:\s*\d+°C)/gi, '<span class="hl-temp-bat">$1</span>')

  // Highlight daemon start/stop
  if (line.includes('started')) body = `<span class="hl-start">${body}</span>`

  return body
}

function lineClass(line) {
  if (line.includes('Gaming') || line.includes('Game Launch Boost')) return 'log-line-gaming'
  if (line.includes('Thermal Tier')) return 'log-line-thermal'
  if (line.includes('started')) return 'log-line-start'
  return ''
}

function handleUserScroll() {
  if (!logContainer.value) return
  const el = logContainer.value
  const isAtBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 30

  // If user manually scrolls up, pause autoScroll
  if (!isAtBottom) {
    autoScroll.value = false
    isUserScrolling = true
  } else {
    isUserScrolling = false
  }
}

function toggleAutoScroll() {
  autoScroll.value = !autoScroll.value
  if (autoScroll.value) {
    scrollToBottom()
  }
}

function scrollToBottom() {
  nextTick(() => {
    if (logContainer.value) {
      logContainer.value.scrollTop = logContainer.value.scrollHeight
    }
  })
}

async function copyLog() {
  try {
    const text = rawLines.value.join('\n')
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
    if (toast) toast('Log copied to clipboard!')
  } catch {
    if (toast) toast('Failed to copy log')
  }
}

async function saveLog() {
  const msg = await store.exportLogs()
  if (toast) toast(msg)
}

watch(() => store.logs, () => {
  if (autoScroll.value && !isUserScrolling) {
    scrollToBottom()
  }
})

onMounted(() => {
  scrollToBottom()
})
</script>

<style scoped>
.log-search-input {
  width: 100%;
  background: var(--surface-container-lowest);
  border: 1px solid var(--surface-container-high);
  color: var(--on-surface);
  font-size: 12px;
  padding: 6px 28px 6px 10px;
  border-radius: 8px;
  outline: none;
  box-sizing: border-box;
}

.log-search-input:focus {
  border-color: var(--primary);
}

.clear-search {
  position: absolute;
  right: 8px;
  top: 50%;
  transform: translateY(-50%);
  color: var(--on-surface-variant);
  font-size: 12px;
  cursor: pointer;
}

.filter-chip {
  background: var(--surface-container-high);
  border: 1px solid transparent;
  color: var(--on-surface-variant);
  font-size: 11px;
  font-weight: 500;
  padding: 4px 10px;
  border-radius: 12px;
  cursor: pointer;
  white-space: nowrap;
  transition: all 0.2s ease;
}

.filter-chip.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-color: var(--primary);
  font-weight: 600;
}

.log-box {
  background: var(--surface-container-lowest);
  border: 1px solid var(--surface-container-high);
  padding: 10px;
  border-radius: 12px;
  font-size: 11px;
  font-family: var(--font-mono);
  line-height: 1.6;
  flex: 1;
  overflow-y: auto;
  color: var(--on-surface-variant);
  margin: 0;
  box-sizing: border-box;
}

.log-line {
  word-break: break-all;
  white-space: pre-wrap;
  padding: 3px 0;
  border-bottom: 1px dashed rgba(255, 255, 255, 0.03);
  display: flex;
  gap: 8px;
}

.log-time {
  color: var(--on-surface-variant);
  opacity: 0.55;
  font-size: 10px;
  flex-shrink: 0;
  user-select: none;
}

.log-body {
  flex: 1;
}

/* Syntax Highlights */
:deep(.hl-profile) {
  color: var(--primary);
  font-weight: 700;
}

:deep(.hl-mode) {
  color: #d0bcff;
  font-weight: 600;
}

:deep(.hl-thermal) {
  color: #ffb74d;
  font-weight: 600;
}

:deep(.hl-temp-cpu) {
  color: #ef5350;
}

:deep(.hl-temp-bat) {
  color: #ffa726;
}

:deep(.hl-start) {
  color: #66bb6a;
  font-weight: 600;
}

.live-dot {
  width: 7px;
  height: 7px;
  background-color: #66bb6a;
  border-radius: 50%;
  display: inline-block;
  box-shadow: 0 0 8px #66bb6a;
  animation: pulse-green 2s infinite;
}

@keyframes pulse-green {
  0% { transform: scale(0.95); opacity: 0.8; }
  50% { transform: scale(1.15); opacity: 1; }
  100% { transform: scale(0.95); opacity: 0.8; }
}
</style>
