<template>
  <!-- Daemon Status Banner -->
  <div class="md3-banner">
    <div style="display: flex; align-items: center; gap: 12px;">
      <img
        :src="store.isRunning ? activeImg : sleepImg"
        style="width: 52px; height: 52px; border-radius: 14px; object-fit: cover; border: 1px solid var(--surface-container-highest); flex-shrink: 0;"
        alt="HyperCore Mascot"
      >
      <div>
        <div style="font-size: 15px; font-weight: 700;">Tanzanite HyperCore</div>
        <div style="font-size: 11px; opacity: 0.85; margin-top: 1px;">
          {{ store.isRunning ? `Daemon PID: ${store.daemonPid}` : 'Service inactive' }}
        </div>
      </div>
    </div>
    <span class="badge-pill" :class="store.isRunning ? 'green' : 'red'">
      {{ store.isRunning ? 'Running' : 'Stopped' }}
    </span>
  </div>

  <!-- Engine status -->
  <div class="section-title">Engine status</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="star" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Active profile</div>
          <div class="row-sub">Current profile mode</div>
        </div>
      </div>
      <div class="row-val c-purple" style="font-weight: 700;">{{ store.activeProfile }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="thermal" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Thermal tier</div>
          <div class="row-sub">Thermal state & throttle limit</div>
        </div>
      </div>
      <div class="row-val" :class="store.thermalColor">{{ store.thermalTier }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="cpu" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">CPU governor</div>
          <div class="row-sub">Scaling governor</div>
        </div>
      </div>
      <div class="row-val">{{ store.cpuGov }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="gpu" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">GPU load / floor</div>
          <div class="row-sub">MediaTek GED floor</div>
        </div>
      </div>
      <div class="row-val">{{ store.gpuInfo }}</div>
    </div>
  </div>

  <!-- Hardware & memory -->
  <div class="section-title">Hardware & memory</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge tertiary">
          <Icons name="thermal" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Temperatures</div>
          <div class="row-sub">CPU & battery sensors</div>
        </div>
      </div>
      <div class="row-val" :class="store.tempColor">{{ tempDisplay }}</div>
    </div>

    <div class="md3-list-row" style="flex-direction: column; align-items: stretch;">
      <div style="display: flex; align-items: center; justify-content: space-between; width: 100%;">
        <div class="row-left">
          <div class="icon-badge secondary">
            <Icons name="memory" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">RAM usage</div>
            <div class="row-sub">Physical memory</div>
          </div>
        </div>
        <div class="row-val">{{ store.ramUsage }}</div>
      </div>
      <div class="progress-track" v-if="store.ramPercent > 0">
        <div class="progress-fill" :class="ramColor" :style="{ width: `${store.ramPercent}%` }"></div>
      </div>
    </div>

    <div class="md3-list-row" style="flex-direction: column; align-items: stretch;">
      <div style="display: flex; align-items: center; justify-content: space-between; width: 100%;">
        <div class="row-left">
          <div class="icon-badge secondary">
            <Icons name="memory" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">ZRAM / Swap</div>
            <div class="row-sub">Compressed swap</div>
          </div>
        </div>
        <div class="row-val">{{ store.zramUsage }}</div>
      </div>
      <div class="progress-track" v-if="store.zramPercent > 0">
        <div class="progress-fill blue" :style="{ width: `${store.zramPercent}%` }"></div>
      </div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge secondary">
          <Icons name="storage" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">I/O queue</div>
          <div class="row-sub">Scheduler & read-ahead buffer</div>
        </div>
      </div>
      <div class="row-val">{{ store.ioInfo }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">System uptime</div>
          <div class="row-sub">Uptime since boot</div>
        </div>
      </div>
      <div class="row-val">{{ store.uptime }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="star" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Module info</div>
          <div class="row-sub">{{ store.chipset }}</div>
        </div>
      </div>
      <div class="row-val c-purple" style="font-weight: 700;">{{ store.moduleVersion }}</div>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'
import activeImg from '@/assets/hypercore_active.jpg'
import sleepImg from '@/assets/hypercore_sleep.jpg'

const store = useHyperStore()

const tempDisplay = computed(() => {
  const c = store.cpuTemp > 0 ? `${store.cpuTemp}°C` : '—'
  const b = store.batTemp > 0 ? `${store.batTemp}°C` : '—'
  return `${c} / ${b}`
})

const ramColor = computed(() => {
  if (store.ramPercent >= 85) return 'red'
  if (store.ramPercent >= 70) return 'amber'
  return 'green'
})
</script>
