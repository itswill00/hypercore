<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">System log</div>
        <div class="page-header-sub">HyperCore By @itswill00</div>
      </div>
      <div style="display: flex; gap: 8px; align-items: center;">
        <button class="btn-md3 btn-md3-secondary" style="padding: 6px 12px; font-size: 12px;" title="Copy all logs" @click="copyLog">
          Copy
        </button>
        <button class="btn-md3 btn-md3-primary" style="padding: 6px 12px; font-size: 12px;" title="Export log" @click="saveLog">
          Save
        </button>
      </div>
    </div>

    <!-- Scroll Content -->
    <div class="content-area" style="display: flex; flex-direction: column; height: 100%; gap: 10px;">
      <div style="display: flex; align-items: center; justify-content: space-between; padding: 0 4px;">
        <span style="font-size: 12px; color: var(--on-surface-variant); font-weight: 500;">Console output</span>
        <span style="font-size: 11px; font-family: var(--font-mono); color: var(--on-surface-variant); opacity: 0.7;">
          {{ parsedLogs.length }} entries · live
        </span>
      </div>

      <!-- Pure Terminal Box -->
      <div
        ref="logContainer"
        class="terminal-box"
        @scroll="handleScroll"
      >
        <div
          v-for="(item, idx) in parsedLogs"
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
  </div>
</template>

<script setup>
import { ref, computed, inject, watch, nextTick, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
const toast = inject('toast')

const logContainer = ref(null)
const autoScroll = ref(true)

const parsedLogs = computed(() => {
  if (!store.logs) return []
  const rawLines = store.logs.split('\n').filter(l => l.trim().length > 0)

  return rawLines.map(line => {
    // Matches: [2026-07-30 23:30:21.142] [INFO ] [DAEMON  ] Message text OR older format
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

watch(() => store.logs, () => {
  scrollToBottom()
})

onMounted(() => {
  if (logContainer.value) {
    logContainer.value.scrollTop = logContainer.value.scrollHeight
  }
})
</script>

<style scoped>
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
