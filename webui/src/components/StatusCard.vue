<template>
  <!-- Daemon Status Banner -->
  <div class="md3-banner">
    <div style="display: flex; align-items: center; gap: 12px;">
      <div class="icon-badge secondary" style="width: 42px; height: 42px; border-radius: 14px;">
        <Icons name="star" :size="22" />
      </div>
      <div>
        <div style="font-size: 15px; font-weight: 700;">Tanzanite HyperCore</div>
        <div style="font-size: 11px; opacity: 0.85; margin-top: 1px;">
          {{ store.isRunning ? `Daemon Active (PID ${store.daemonPid})` : 'Daemon Stopped' }}
        </div>
      </div>
    </div>
    <span class="badge-pill" :class="store.isRunning ? 'green' : 'red'">
      {{ store.isRunning ? 'Running' : 'Stopped' }}
    </span>
  </div>

  <!-- System Performance -->
  <div class="section-title">System Performance</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="star" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Active Profile</div>
          <div class="row-sub">Daemon runtime mode</div>
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
          <div class="row-title">Thermal Tier</div>
          <div class="row-sub">Hysteresis thermal state</div>
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
          <div class="row-title">CPU Governor</div>
          <div class="row-sub">Scaling governor (Cores {{ store.cpuCores }})</div>
        </div>
      </div>
      <div class="row-val">{{ store.cpuGov }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="cpu" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Little Cores (0–5)</div>
          <div class="row-sub">Frequency range & current clock</div>
        </div>
      </div>
      <div class="row-val">{{ store.cpuLittle }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="cpu" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Big Cores (6–7)</div>
          <div class="row-sub">Cortex-A76 performance cluster</div>
        </div>
      </div>
      <div class="row-val">{{ store.cpuBig }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="gpu" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">GPU Load / Floor</div>
          <div class="row-sub">MediaTek GED GPU floor</div>
        </div>
      </div>
      <div class="row-val">{{ store.gpuInfo }}</div>
    </div>
  </div>

  <!-- Memory & Storage -->
  <div class="section-title">Memory & Storage</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge secondary">
          <Icons name="memory" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">RAM Usage</div>
          <div class="row-sub">Physical memory allocation</div>
        </div>
      </div>
      <div class="row-val">{{ store.ramUsage }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge secondary">
          <Icons name="memory" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">ZRAM / Swap</div>
          <div class="row-sub">Compressed ZRAM swap memory</div>
        </div>
      </div>
      <div class="row-val">{{ store.zramUsage }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge secondary">
          <Icons name="storage" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">I/O Queue & Read-Ahead</div>
          <div class="row-sub">UFS storage queue tuning</div>
        </div>
      </div>
      <div class="row-val">{{ store.ioInfo }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge secondary">
          <Icons name="storage" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Swappiness / VFS Cache</div>
          <div class="row-sub">Virtual memory parameters</div>
        </div>
      </div>
      <div class="row-val">{{ store.vmInfo }}</div>
    </div>
  </div>

  <!-- Power & Thermal -->
  <div class="section-title">Power & Thermal</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge tertiary">
          <Icons name="thermal" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">CPU / Battery Temp</div>
          <div class="row-sub">Thermal zone sensors</div>
        </div>
      </div>
      <div class="row-val" :class="store.tempColor">{{ tempDisplay }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge tertiary">
          <Icons name="power" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Battery Status</div>
          <div class="row-sub">State & charge percentage</div>
        </div>
      </div>
      <div class="row-val" :class="store.batColor">{{ batDisplay }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge tertiary">
          <Icons name="power" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Current / Voltage</div>
          <div class="row-sub">Realtime charge rate</div>
        </div>
      </div>
      <div class="row-val">{{ store.batRate }} / {{ store.batVolt }}</div>
    </div>
  </div>

  <!-- Device Info -->
  <div class="section-title">Device Info</div>
  <div class="md3-list-group">
    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Module Version</div>
          <div class="row-sub">Tanzanite HyperCore</div>
        </div>
      </div>
      <div class="row-val c-purple" style="font-weight: 700;">{{ store.moduleVersion }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Chipset</div>
          <div class="row-sub">System-on-Chip</div>
        </div>
      </div>
      <div class="row-val">{{ store.chipset }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Kernel</div>
          <div class="row-sub">Linux kernel release</div>
        </div>
      </div>
      <div class="row-val">{{ store.kernelVersion }}</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">Android SDK & SELinux</div>
          <div class="row-sub">System environment</div>
        </div>
      </div>
      <div class="row-val">{{ store.androidSdk }} ({{ store.selinux }})</div>
    </div>

    <div class="md3-list-row">
      <div class="row-left">
        <div class="icon-badge">
          <Icons name="device" :size="18" />
        </div>
        <div class="row-meta">
          <div class="row-title">System Uptime</div>
          <div class="row-sub">Uptime since boot</div>
        </div>
      </div>
      <div class="row-val">{{ store.uptime }}</div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()

onMounted(() => {
  store.fetchDeviceInfo()
})

const tempDisplay = computed(() => {
  const c = store.cpuTemp > 0 ? `${store.cpuTemp}°C` : '—'
  const b = store.batTemp > 0 ? `${store.batTemp}°C` : '—'
  return `${c} / ${b}`
})

const batDisplay = computed(() => {
  return store.batStatus + (store.batLevel ? ` (${store.batLevel}%)` : '')
})
</script>
