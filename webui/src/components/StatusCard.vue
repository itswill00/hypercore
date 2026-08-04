<template>
  <div class="status-banner">
    <div class="status-banner-left">
      <div class="status-title">Tanzanite HyperCore</div>
      <div class="status-sub">
        {{ store.isRunning ? `Running (PID ${store.daemonPid}) • ${store.moduleVersion}` : 'Service inactive' }}
      </div>
    </div>
    <div class="status-banner-badges">
      <span class="status-pill" :class="store.isRunning ? 'is-running' : 'is-stopped'">
        {{ store.isRunning ? 'Running' : 'Stopped' }}
      </span>
      <span v-if="store.isRunning" class="profile-pill">
        {{ store.activeProfile || 'Interactive' }}
      </span>
    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
</script>

<style scoped>
.status-banner {
  background: var(--surface-container-high, #26272e);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 16px;
  margin-bottom: 14px;
  padding: 14px 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.status-banner-left {
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.status-title {
  font-size: 15px;
  font-weight: 700;
  color: var(--on-surface, #f0f2f5);
}

.status-sub {
  font-size: 11px;
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.85;
  margin-top: 2px;
}

.status-banner-badges {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-shrink: 0;
}

.status-pill {
  font-size: 10px;
  font-weight: 700;
  padding: 3px 10px;
  border-radius: 20px;
  text-transform: uppercase;
  letter-spacing: 0.4px;
}

.status-pill.is-running {
  background: rgba(160, 178, 198, 0.16);
  color: var(--blue, #a0b2c6);
  border: 1px solid rgba(160, 178, 198, 0.25);
}

.status-pill.is-stopped {
  background: rgba(229, 115, 115, 0.16);
  color: var(--red, #e57373);
  border: 1px solid rgba(229, 115, 115, 0.25);
}

.profile-pill {
  font-size: 10px;
  font-weight: 700;
  padding: 3px 10px;
  border-radius: 20px;
  background: var(--surface-container, #1d1e23);
  color: var(--on-surface, #f0f2f5);
  border: 1px solid var(--outline-variant, #383a42);
}
</style>
