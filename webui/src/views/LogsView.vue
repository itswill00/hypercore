<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Sticky Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">System Logs</div>
        <div class="page-header-sub">Realtime daemon runtime events</div>
      </div>
      <div style="display: flex; gap: 8px; align-items: center;">
        <button class="btn-md3 btn-md3-secondary btn-icon-only" @click="store.refresh()">
          <Icons name="refresh" :size="16" />
        </button>
        <button class="btn-md3 btn-md3-primary" style="padding: 6px 12px;" @click="saveLog">
          <Icons name="logs" :size="14" />
          <span>Save Log</span>
        </button>
      </div>
    </div>

    <!-- Scroll Content -->
    <div class="content-area">
      <div class="md3-card">
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px;">
          <span style="font-size: 11px; font-weight: 700; color: var(--primary); text-transform: uppercase;">LIVE LOG CONSOLE</span>
          <span style="font-size: 10px; font-family: var(--font-mono); color: var(--on-surface-variant);">/sdcard/Android/hypercore.log</span>
        </div>
        <pre class="log-box" style="background: var(--surface-container-lowest); border: 1px solid var(--surface-container-high); padding: 12px; border-radius: 12px; font-size: 11px; font-family: var(--font-mono); line-height: 1.55; height: 400px; overflow-y: auto; white-space: pre-wrap; word-break: break-all; color: var(--on-surface-variant); margin: 0;">{{ store.logs }}</pre>
      </div>
    </div>
  </div>
</template>

<script setup>
import { inject, watch, nextTick } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

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
