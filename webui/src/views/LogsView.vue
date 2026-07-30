<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Sticky Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">System log</div>
        <div class="page-header-sub">HyperCore By @itswill00</div>
      </div>
      <div style="display: flex; gap: 8px; align-items: center;">
        <button class="btn-md3 btn-md3-secondary btn-icon-only" @click="store.refresh()">
          <Icons name="refresh" :size="16" />
        </button>
        <button class="btn-md3 btn-md3-primary" style="padding: 6px 12px;" @click="saveLog">
          <Icons name="logs" :size="14" />
          <span>Save log</span>
        </button>
      </div>
    </div>

    <!-- Scroll Content -->
    <div class="content-area">
      <div class="md3-card">
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px;">
          <span style="font-size: 12px; font-weight: 600; color: var(--on-surface-variant);">Live log console</span>
          <span style="font-size: 10px; font-family: var(--font-mono); color: var(--on-surface-variant);">/sdcard/Android/hypercore.log</span>
        </div>
        
        <div class="log-box" style="background: var(--surface-container-lowest); border: 1px solid var(--surface-container-high); padding: 12px; border-radius: 12px; font-size: 11px; font-family: var(--font-mono); line-height: 1.6; height: 420px; overflow-y: auto; color: var(--on-surface-variant); margin: 0;">
          <div
            v-for="(line, idx) in logLines"
            :key="idx"
            class="log-line"
            :class="lineClass(line)"
          >
            {{ line }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, inject, watch, nextTick } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const logLines = computed(() => {
  if (!store.logs) return []
  return store.logs.split('\n').filter(l => l.trim().length > 0)
})

function lineClass(line) {
  if (line.includes('Gaming') || line.includes('Game Launch Boost')) return 'log-gaming'
  if (line.includes('Thermal Tier')) return 'log-thermal'
  if (line.includes('Touch') || line.includes('Interactive')) return 'log-profile'
  if (line.includes('started')) return 'log-start'
  if (line.includes('stopped')) return 'log-stop'
  return ''
}

async function saveLog() {
  const msg = await store.exportLogs()
  if (toast) toast(msg)
}

watch(() => store.logs, async () => {
  await nextTick()
  const el = document.querySelector('.log-box')
  if (el) el.scrollTop = el.scrollHeight
})
</script>

<style scoped>
.log-line {
  word-break: break-all;
  white-space: pre-wrap;
  padding: 1px 0;
  border-bottom: 1px dashed rgba(255, 255, 255, 0.03);
}

.log-gaming {
  color: #81c784;
  font-weight: 600;
}

.log-thermal {
  color: #ffb74d;
  font-weight: 500;
}

.log-profile {
  color: #d0bcff;
}

.log-start {
  color: #64b5f6;
  font-weight: 600;
}

.log-stop {
  color: #e57373;
}
</style>
