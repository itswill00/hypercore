<template>
  <div class="md3-banner" style="margin-bottom: 14px; padding: 14px 16px;">
    <div>
      <div style="font-size: 15px; font-weight: 700; color: var(--on-secondary-container);">Tanzanite HyperCore</div>
      <div style="font-size: 11px; opacity: 0.85; margin-top: 2px;">
        {{ store.isRunning ? `Running (PID ${store.daemonPid}) • ${store.moduleVersion}` : 'Service inactive' }}
      </div>
    </div>
    <span class="badge-pill" :style="store.isRunning ? 'background: rgba(160, 178, 198, 0.16); color: #a0b2c6;' : 'background: rgba(229, 115, 115, 0.16); color: #e57373;'">
      {{ store.isRunning ? 'Running' : 'Stopped' }}
    </span>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
</script>
