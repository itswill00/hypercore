<template>
  <!-- System status -->
  <div class="card">
    <div class="card-title">System status</div>
    <div class="row"><span class="lbl">Daemon</span><span class="val" :class="store.isRunning ? 'c-green' : 'c-red'">{{ store.isRunning ? `Running (PID ${store.daemonPid})` : 'Stopped' }}</span></div>
    <div class="row"><span class="lbl">Active profile</span><span class="val">{{ store.activeProfile }}</span></div>
    <div class="row"><span class="lbl">Thermal tier</span><span class="val" :class="store.thermalColor">{{ store.thermalTier }}</span></div>
    <div class="row"><span class="lbl">CPU governor</span><span class="val">{{ store.cpuGov }} (cores {{ store.cpuCores }})</span></div>
    <div class="row"><span class="lbl">Little cores (0–5)</span><span class="val">{{ store.cpuLittle }}</span></div>
    <div class="row"><span class="lbl">Big cores (6–7)</span><span class="val">{{ store.cpuBig }}</span></div>
    <div class="row"><span class="lbl">GPU load / floor</span><span class="val">{{ store.gpuInfo }}</span></div>
    <div class="row"><span class="lbl">System load</span><span class="val">{{ store.sysLoad }}</span></div>
  </div>

  <!-- Memory & Storage -->
  <div class="card">
    <div class="card-title">Memory & storage</div>
    <div class="row"><span class="lbl">RAM usage</span><span class="val">{{ store.ramUsage }}</span></div>
    <div class="row"><span class="lbl">ZRAM / Swap</span><span class="val">{{ store.zramUsage }}</span></div>
    <div class="row"><span class="lbl">I/O scheduler / read-ahead</span><span class="val">{{ store.ioInfo }}</span></div>
    <div class="row"><span class="lbl">Swappiness / VFS</span><span class="val">{{ store.vmInfo }}</span></div>
  </div>

  <!-- Power & Thermal -->
  <div class="card">
    <div class="card-title">Power & thermal</div>
    <div class="row"><span class="lbl">CPU / battery temp</span><span class="val" :class="store.tempColor">{{ tempDisplay }}</span></div>
    <div class="row"><span class="lbl">Battery</span><span class="val" :class="store.batColor">{{ batDisplay }}</span></div>
    <div class="row"><span class="lbl">Current / voltage</span><span class="val">{{ store.batRate }} / {{ store.batVolt }}</span></div>
  </div>

  <!-- Device info -->
  <div class="card">
    <div class="card-title">Device info</div>
    <div class="row"><span class="lbl">Module version</span><span class="val c-accent">{{ store.moduleVersion }}</span></div>
    <div class="row"><span class="lbl">Chipset</span><span class="val">{{ store.chipset }}</span></div>
    <div class="row"><span class="lbl">Kernel</span><span class="val">{{ store.kernelVersion }}</span></div>
    <div class="row"><span class="lbl">Android SDK</span><span class="val">{{ store.androidSdk }}</span></div>
    <div class="row"><span class="lbl">SELinux</span><span class="val">{{ store.selinux }}</span></div>
    <div class="row"><span class="lbl">System uptime</span><span class="val">{{ store.uptime }}</span></div>
  </div>
</template>

<script setup>
import { computed, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

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
