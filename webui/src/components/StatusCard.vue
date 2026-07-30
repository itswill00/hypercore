<template>
  <!-- Daemon Status Banner -->
  <div class="md3-group p-4 bg-secondary-container text-on-secondary-container flex items-center justify-between shadow-sm">
    <div class="flex items-center gap-3.5">
      <div class="w-12 h-12 rounded-2xl bg-primary-container text-on-primary-container flex items-center justify-center shrink-0">
        <Icons name="star" :size="24" />
      </div>
      <div>
        <h2 class="text-base font-semibold">Tanzanite HyperCore</h2>
        <p class="text-xs opacity-80 mt-0.5">
          {{ store.isRunning ? `Daemon Active (PID ${store.daemonPid})` : 'Daemon Stopped' }}
        </p>
      </div>
    </div>
    <span
      class="px-3 py-1 text-xs font-semibold rounded-full"
      :class="store.isRunning ? 'bg-green-500/20 text-green-300' : 'bg-red-500/20 text-red-300'"
    >
      {{ store.isRunning ? 'Running' : 'Stopped' }}
    </span>
  </div>

  <!-- System Performance -->
  <div class="mb-4">
    <h3 class="section-title">SYSTEM PERFORMANCE</h3>
    <div class="md3-group">
      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="star" :size="18" />
          </div>
          <div>
            <div class="item-title">Active Profile</div>
            <div class="item-sub">Daemon runtime mode</div>
          </div>
        </div>
        <div class="item-val font-semibold text-primary">{{ store.activeProfile }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="thermal" :size="18" />
          </div>
          <div>
            <div class="item-title">Thermal Tier</div>
            <div class="item-sub">Hysteresis thermal state</div>
          </div>
        </div>
        <div class="item-val" :class="store.thermalColor">{{ store.thermalTier }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="cpu" :size="18" />
          </div>
          <div>
            <div class="item-title">CPU Governor</div>
            <div class="item-sub">Scaling governor (Cores {{ store.cpuCores }})</div>
          </div>
        </div>
        <div class="item-val">{{ store.cpuGov }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="cpu" :size="18" />
          </div>
          <div>
            <div class="item-title">Little Cores (0–5)</div>
            <div class="item-sub">Frequency range & current clock</div>
          </div>
        </div>
        <div class="item-val">{{ store.cpuLittle }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="cpu" :size="18" />
          </div>
          <div>
            <div class="item-title">Big Cores (6–7)</div>
            <div class="item-sub">Cortex-A76 performance cluster</div>
          </div>
        </div>
        <div class="item-val">{{ store.cpuBig }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="gpu" :size="18" />
          </div>
          <div>
            <div class="item-title">GPU Load / Floor</div>
            <div class="item-sub">MediaTek GED GPU floor</div>
          </div>
        </div>
        <div class="item-val">{{ store.gpuInfo }}</div>
      </div>
    </div>
  </div>

  <!-- Memory & Storage -->
  <div class="mb-4">
    <h3 class="section-title">MEMORY & STORAGE</h3>
    <div class="md3-group">
      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge secondary">
            <Icons name="memory" :size="18" />
          </div>
          <div>
            <div class="item-title">RAM Usage</div>
            <div class="item-sub">Physical memory allocation</div>
          </div>
        </div>
        <div class="item-val">{{ store.ramUsage }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge secondary">
            <Icons name="memory" :size="18" />
          </div>
          <div>
            <div class="item-title">ZRAM / Swap</div>
            <div class="item-sub">Compressed ZRAM swap memory</div>
          </div>
        </div>
        <div class="item-val">{{ store.zramUsage }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge secondary">
            <Icons name="storage" :size="18" />
          </div>
          <div>
            <div class="item-title">I/O Queue & Read-Ahead</div>
            <div class="item-sub">UFS storage queue tuning</div>
          </div>
        </div>
        <div class="item-val">{{ store.ioInfo }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge secondary">
            <Icons name="storage" :size="18" />
          </div>
          <div>
            <div class="item-title">Swappiness / VFS Cache</div>
            <div class="item-sub">Virtual memory parameters</div>
          </div>
        </div>
        <div class="item-val">{{ store.vmInfo }}</div>
      </div>
    </div>
  </div>

  <!-- Power & Thermal -->
  <div class="mb-4">
    <h3 class="section-title">POWER & THERMAL</h3>
    <div class="md3-group">
      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge tertiary">
            <Icons name="thermal" :size="18" />
          </div>
          <div>
            <div class="item-title">CPU / Battery Temp</div>
            <div class="item-sub">Thermal zone sensors</div>
          </div>
        </div>
        <div class="item-val" :class="store.tempColor">{{ tempDisplay }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge tertiary">
            <Icons name="power" :size="18" />
          </div>
          <div>
            <div class="item-title">Battery Status</div>
            <div class="item-sub">State & charge percentage</div>
          </div>
        </div>
        <div class="item-val" :class="store.batColor">{{ batDisplay }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge tertiary">
            <Icons name="power" :size="18" />
          </div>
          <div>
            <div class="item-title">Current / Voltage</div>
            <div class="item-sub">Realtime charge rate</div>
          </div>
        </div>
        <div class="item-val">{{ store.batRate }} / {{ store.batVolt }}</div>
      </div>
    </div>
  </div>

  <!-- Device Metadata -->
  <div class="mb-4">
    <h3 class="section-title">DEVICE INFO</h3>
    <div class="md3-group">
      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="device" :size="18" />
          </div>
          <div>
            <div class="item-title">Module Version</div>
            <div class="item-sub">Tanzanite HyperCore</div>
          </div>
        </div>
        <div class="item-val text-primary font-semibold">{{ store.moduleVersion }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="device" :size="18" />
          </div>
          <div>
            <div class="item-title">Chipset</div>
            <div class="item-sub">Target System-on-Chip</div>
          </div>
        </div>
        <div class="item-val">{{ store.chipset }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="device" :size="18" />
          </div>
          <div>
            <div class="item-title">Kernel</div>
            <div class="item-sub">Linux kernel release</div>
          </div>
        </div>
        <div class="item-val">{{ store.kernelVersion }}</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="device" :size="18" />
          </div>
          <div>
            <div class="item-title">Android SDK & SELinux</div>
            <div class="item-sub">System security & version</div>
          </div>
        </div>
        <div class="item-val">{{ store.androidSdk }} ({{ store.selinux }})</div>
      </div>

      <div class="md3-list-item">
        <div class="flex items-center gap-3">
          <div class="icon-badge">
            <Icons name="device" :size="18" />
          </div>
          <div>
            <div class="item-title">System Uptime</div>
            <div class="item-sub">Time since last boot</div>
          </div>
        </div>
        <div class="item-val">{{ store.uptime }}</div>
      </div>
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
