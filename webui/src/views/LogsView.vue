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
          {{ logLines.length }} lines · live
        </span>
      </div>

      <!-- Pure Terminal Box -->
      <div
        ref="logContainer"
        class="terminal-box"
        @scroll="handleScroll"
      >
        <div
          v-for="(line, idx) in logLines"
          :key="idx"
          class="terminal-line"
        >
          <span class="t-stamp" v-if="getTimestamp(line)">{{ getTimestamp(line) }}</span>
          <span class="t-text" :class="getTextClass(line)">{{ getBody(line) }}</span>
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

const logLines = computed(() => {
  if (!store.logs) return []
  return store.logs.split('\n').filter(l => l.trim().length > 0)
})

function getTimestamp(line) {
  const m = line.match(/^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})/)
  return m ? m[1] : ''
}

function getBody(line) {
  return line.replace(/^\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\s*/, '')
}

function getTextClass(line) {
  if (line.includes('Gaming')) return 'txt-gaming'
  if (line.includes('Thermal Tier')) return 'txt-thermal'
  if (line.includes('started')) return 'txt-start'
  return ''
}

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
    const text = logLines.value.join('\n')
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
  gap: 10px;
  word-break: break-all;
  white-space: pre-wrap;
  padding: 1px 0;
}

.t-stamp {
  color: var(--on-surface-variant);
  opacity: 0.45;
  font-size: 10px;
  flex-shrink: 0;
  user-select: none;
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
