<template>
  <div class="md3-banner" style="margin-bottom: 14px;">
    <div style="display: flex; align-items: center; gap: 14px;">
      <img
        :src="store.isRunning ? activeImg : sleepImg"
        style="width: 56px; height: 56px; border-radius: 14px; object-fit: cover; border: 1px solid var(--surface-container-highest); flex-shrink: 0;"
        alt="HyperCore Mascot"
      >
      <div>
        <div style="font-size: 15px; font-weight: 700; color: var(--on-secondary-container);">Tanzanite HyperCore</div>
        <div style="font-size: 11px; opacity: 0.85; margin-top: 2px;">
          {{ store.isRunning ? `Running (PID ${store.daemonPid}) • ${store.moduleVersion}` : 'Service inactive' }}
        </div>
      </div>
    </div>
    <span class="badge-pill" :class="store.isRunning ? 'green' : 'red'">
      {{ store.isRunning ? 'Running' : 'Stopped' }}
    </span>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import activeImg from '@/assets/hypercore_active.jpg'
import sleepImg from '@/assets/hypercore_sleep.jpg'

const store = useHyperStore()
</script>
