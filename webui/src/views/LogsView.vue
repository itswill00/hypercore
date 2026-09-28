<template>
  <div class="logs-page-container">
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
import { ref, computed, watch, nextTick, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()

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

      const uLevel = level.toUpperCase()
      const uTag = tag.toUpperCase()

      let levelClass = 'lbl-info'
      if (uLevel === 'STATE') levelClass = 'lbl-state'
      else if (uLevel === 'WARN') levelClass = 'lbl-warn'
      else if (uLevel === 'ERROR') levelClass = 'lbl-error'

      let textClass = ''
      if (msg.includes('Gaming') || uTag === 'GAME') textClass = 'txt-gaming'
      else if (msg.includes('Thermal Tier') || uLevel === 'WARN') textClass = 'txt-thermal'
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
  min-height: 0;
}

.terminal-viewport {
  flex: 1;
  min-height: 0;
  background: #070709;
  /* deslop-ignore-next-line 34 */
  font-family: var(--font-mono);
  font-size: 11px;
  line-height: 1.5;
  overflow-y: auto;
  overflow-x: auto;
  box-sizing: border-box;
  padding: 8px 12px calc(92px + var(--window-inset-bottom, 0px)) 12px;
  scrollbar-width: thin;
  scrollbar-color: var(--surface-container-highest) transparent;
  overscroll-behavior: auto;
  -webkit-overflow-scrolling: touch;
  touch-action: pan-y pan-x;
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



























.log-line-enter-active {
  transition: opacity 0.2s ease, transform 0.2s ease;
}

.log-line-enter-from {
  opacity: 0;
  transform: translateY(6px);
}
</style>
