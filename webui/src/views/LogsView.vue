<template>
  <div class="page-logs pb-safe-nav overflow-y-auto h-full scrollbar-hidden">
    <!-- Header -->
    <div class="sticky top-0 z-10 bg-surface/90 backdrop-blur-md px-4 py-3 border-b border-surface-container-high mb-4">
      <div class="flex items-center justify-between">
        <div>
          <h1 class="text-lg font-bold text-on-surface">System Logs</h1>
          <p class="text-xs text-on-surface-variant">Realtime HyperCore daemon runtime events</p>
        </div>
        <div class="flex gap-2">
          <button class="md3-btn md3-btn-secondary p-2 rounded-full" @click="store.refresh()">
            <Icons name="refresh" :size="18" />
          </button>
          <button class="md3-btn md3-btn-primary py-1.5 px-3 text-xs" @click="saveLog">
            <Icons name="logs" :size="16" />
            <span>Save Log</span>
          </button>
        </div>
      </div>
    </div>

    <div class="px-4 max-w-md mx-auto">
      <div class="md3-group p-4">
        <div class="flex items-center justify-between mb-3">
          <span class="text-xs font-semibold text-primary">LIVE LOG CONSOLE</span>
          <span class="text-[11px] font-mono text-on-surface-variant">/sdcard/Android/hypercore.log</span>
        </div>
        <pre class="log-box font-mono text-[11px] leading-relaxed p-3.5 bg-surface-container-lowest rounded-xl border border-surface-container-high text-on-surface-variant overflow-y-auto h-[420px] whitespace-pre-wrap break-all">{{ store.logs }}</pre>
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
