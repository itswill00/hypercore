<template>
  <div class="engine-card">
    <!-- Card Header -->
    <div class="engine-header">
      <div class="engine-title-group">
        <div class="pulse-icon-container" :class="profileClass">
          <span class="pulse-ring"></span>
          <span class="pulse-dot"></span>
        </div>
        <div>
          <div class="engine-title">Smart Adaptive Engine</div>
          <div class="engine-subtitle">Automated EAS & Telemetry Profiler</div>
        </div>
      </div>

      <div class="engine-pill" :class="profileClass">
        <span class="pill-dot"></span>
        {{ activeProfileName }}
      </div>
    </div>

    <!-- Live Telemetry Badges Grid -->
    <div class="engine-metrics-grid">
      <div class="metric-item">
        <span class="metric-label">PROFILE MODE</span>
        <span class="metric-value" :class="profileClass">{{ activeProfileName }}</span>
      </div>

      <div class="metric-item">
        <span class="metric-label">GOVERNOR</span>
        <span class="metric-value">{{ store.cpuGov || 'sugov_ext' }}</span>
      </div>

      <div class="metric-item">
        <span class="metric-label">THERMAL TIER</span>
        <span class="metric-value" :class="store.thermalColor">{{ store.thermalTier || 'Tier 0' }}</span>
      </div>
    </div>

    <!-- Dynamic Mode Description Bar -->
    <div class="engine-footer">
      <span class="footer-icon">⚡</span>
      <span class="footer-text">{{ modeDescription }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()

const activeProfileName = computed(() => {
  if (!store.isRunning) return 'Inactive'
  return store.activeProfile || 'Interactive'
})

const profileClass = computed(() => {
  if (!store.isRunning) return 'mode-inactive'
  const p = (store.activeProfile || '').toLowerCase()
  if (p.includes('gaming')) return 'mode-gaming'
  if (p.includes('sleep')) return 'mode-sleep'
  if (p.includes('thermal')) return 'mode-thermal'
  return 'mode-interactive'
})

const modeDescription = computed(() => {
  if (!store.isRunning) return 'Daemon inactive · Zero power consumption'
  const p = (store.activeProfile || '').toLowerCase()
  if (p.includes('gaming')) {
    return 'High Performance EAS · Mali GPU Always-On · UFS 128 Queue'
  }
  if (p.includes('sleep')) {
    return 'Ultra Low-Power Sleep Barrier · Dynamic 8s Poll Scaling'
  }
  if (p.includes('thermal')) {
    return 'Active Thermal Throttling Mitigation · Hardware Cooldown'
  }
  return 'Adaptive Power & Efficiency · 100% Automated Profile Scaling'
})
</script>

<style scoped>
.engine-card {
  background: linear-gradient(145deg, rgba(26, 23, 34, 0.9), rgba(18, 16, 25, 0.95));
  border: 1px solid rgba(186, 104, 200, 0.25);
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 12px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.35);
  backdrop-filter: blur(10px);
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.engine-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.engine-title-group {
  display: flex;
  align-items: center;
  gap: 10px;
}

.engine-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface);
  letter-spacing: -0.2px;
}

.engine-subtitle {
  font-size: 11px;
  color: var(--on-surface-variant);
  opacity: 0.75;
  margin-top: 1px;
}

/* Pulsing Status Dot */
.pulse-icon-container {
  width: 14px;
  height: 14px;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.pulse-ring {
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  animation: pulseRing 2s cubic-bezier(0.455, 0.03, 0.515, 0.955) infinite;
}

.pulse-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  z-index: 1;
}

@keyframes pulseRing {
  0% { transform: scale(0.8); opacity: 0.8; }
  50% { transform: scale(1.6); opacity: 0; }
  100% { transform: scale(0.8); opacity: 0; }
}

/* Engine Status Pill */
.engine-pill {
  font-size: 10px;
  font-weight: 700;
  padding: 4px 10px;
  border-radius: 20px;
  display: flex;
  align-items: center;
  gap: 5px;
  letter-spacing: 0.4px;
  text-transform: uppercase;
}

.pill-dot {
  width: 5px;
  height: 5px;
  border-radius: 50%;
  background: currentColor;
}

/* Profile Classes */
.mode-gaming .pulse-ring, .mode-gaming .pulse-dot { background: #81c784; }
.mode-gaming.engine-pill { background: rgba(129, 199, 132, 0.15); color: #81c784; border: 1px solid rgba(129, 199, 132, 0.3); }
.mode-gaming.metric-value { color: #81c784; }

.mode-interactive .pulse-ring, .mode-interactive .pulse-dot { background: #ba68c8; }
.mode-interactive.engine-pill { background: rgba(186, 104, 200, 0.15); color: #ba68c8; border: 1px solid rgba(186, 104, 200, 0.3); }
.mode-interactive.metric-value { color: #ba68c8; }

.mode-sleep .pulse-ring, .mode-sleep .pulse-dot { background: #64b5f6; }
.mode-sleep.engine-pill { background: rgba(100, 181, 246, 0.15); color: #64b5f6; border: 1px solid rgba(100, 181, 246, 0.3); }
.mode-sleep.metric-value { color: #64b5f6; }

.mode-thermal .pulse-ring, .mode-thermal .pulse-dot { background: #ffb74d; }
.mode-thermal.engine-pill { background: rgba(255, 183, 77, 0.15); color: #ffb74d; border: 1px solid rgba(255, 183, 77, 0.3); }
.mode-thermal.metric-value { color: #ffb74d; }

.mode-inactive .pulse-ring, .mode-inactive .pulse-dot { background: #e57373; }
.mode-inactive.engine-pill { background: rgba(229, 115, 115, 0.15); color: #e57373; border: 1px solid rgba(229, 115, 115, 0.3); }
.mode-inactive.metric-value { color: #e57373; }

/* Metrics Grid */
.engine-metrics-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  background: rgba(0, 0, 0, 0.25);
  border-radius: 12px;
  padding: 10px;
  border: 1px solid rgba(255, 255, 255, 0.05);
}

.metric-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 2px;
}

.metric-label {
  font-size: 8.5px;
  font-weight: 700;
  color: var(--on-surface-variant);
  opacity: 0.65;
  letter-spacing: 0.5px;
}

.metric-value {
  font-size: 11px;
  font-weight: 700;
  font-family: var(--font-mono);
  color: var(--on-surface);
}

/* Footer Description */
.engine-footer {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 10.5px;
  color: var(--on-surface-variant);
  opacity: 0.85;
  padding-top: 2px;
}

.footer-icon {
  font-size: 11px;
  color: #ffb74d;
}

.footer-text {
  font-weight: 500;
  line-height: 1.3;
}
</style>
