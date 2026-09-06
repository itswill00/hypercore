<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header with Back Button -->
    <div class="page-header">
      <div style="display: flex; align-items: center; gap: 12px;">
        <button class="btn-icon-back" @click="$router.push('/')" title="Back to Dashboard">
          <Icons name="chevron-left" :size="20" />
        </button>
        <div>
          <div class="page-header-title">HyperMoon</div>
          <div class="page-header-sub">Real-Time Performance HUD Overlay</div>
        </div>
      </div>
      <span class="badge-pill" :class="hudStore.isRunning ? 'badge-active' : ''">
        {{ hudStore.isRunning ? 'Active' : 'Standby' }}
      </span>
    </div>

    <div class="content-area">

      <!-- Master Control Hero Card -->
      <div class="hud-master-card">
        <div class="master-card-top">
          <div class="master-icon-group">
            <div class="icon-badge" :class="hudStore.config.visible ? '' : 'secondary'">
              <Icons name="moon" :size="20" />
            </div>
            <div>
              <div class="master-card-title">HUD Overlay Master Switch</div>
              <div class="master-card-sub">
                {{ hudStore.config.visible ? 'Floating on-screen canvas is visible' : 'Overlay is hidden & dormant' }}
              </div>
            </div>
          </div>
          <label class="md3-switch">
            <input
              type="checkbox"
              :checked="hudStore.config.visible"
              :disabled="hudStore.loading"
              @change="onToggleMaster"
            />
            <span class="md3-switch-track">
              <span class="md3-switch-thumb"></span>
            </span>
          </label>
        </div>

        <div class="hud-status-chips">
          <div class="chip-item">
            <span>Daemon: {{ hudStore.daemonPid ? `PID ${hudStore.daemonPid}` : 'Inactive' }}</span>
          </div>
          <div class="chip-item">
            <span>Overlay: {{ hudStore.overlayPid ? `PID ${hudStore.overlayPid}` : 'Inactive' }}</span>
          </div>
          <div class="chip-item" v-if="hudStore.stats.fps && hudStore.stats.fps !== '--'">
            <span>{{ hudStore.stats.fps }} FPS</span>
          </div>
          <div class="chip-item" v-if="hudStore.stats.screen_hz && hudStore.stats.screen_hz !== '--'">
            <span>{{ hudStore.stats.screen_hz }}</span>
          </div>
        </div>
      </div>

      <!-- Smart Game Auto-Trigger Option -->
      <div class="section-title">Smart Automation</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="toggleAutoGaming">
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="games" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Auto-Launch on Game</div>
              <div class="row-sub">Automatically display HUD when games in gamelist are opened</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                v-model="hudStore.config.auto_gaming"
                @change="hudStore.saveConfigDebounced"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>
      </div>

      <!-- Layout & Orientation Style -->
      <div class="section-title">Layout Style</div>
      <div class="md3-list-group" style="padding: 14px;">
        <div class="layout-toggle-container">
          <button
            class="layout-btn"
            :class="{ 'layout-btn-active': hudStore.config.is_horizontal }"
            @click="setLayout(true)"
          >
            <Icons name="more-vertical" :size="16" style="transform: rotate(90deg);" />
            <span>Capsule Pill (Horizontal)</span>
          </button>
          <button
            class="layout-btn"
            :class="{ 'layout-btn-active': !hudStore.config.is_horizontal }"
            @click="setLayout(false)"
          >
            <Icons name="grid" :size="16" />
            <span>Card Stack (Vertical)</span>
          </button>
        </div>

        <div style="display: flex; align-items: center; justify-content: space-between; margin-top: 14px; padding-top: 12px; border-top: 1px solid var(--surface-container-highest);">
          <span style="font-size: 13px; font-weight: 500; color: var(--on-surface-variant);">Text Alignment</span>
          <div class="align-pills">
            <button
              v-for="a in ['left', 'center', 'right']"
              :key="a"
              class="align-pill"
              :class="{ 'align-pill-active': hudStore.config.align === a }"
              @click="setAlignment(a)"
            >
              {{ a.toUpperCase() }}
            </button>
          </div>
        </div>
      </div>

      <!-- Display Metrics Toggles -->
      <div class="section-title">Display Metrics</div>
      <div class="md3-list-group">
        
        <div class="md3-list-row clickable" @click="toggleMetric('show_fps')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="zap" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Frame Rate &amp; Hz</div>
              <div class="row-sub">Real-time FPS counter and display refresh rate</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_fps" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_cpu')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="cpu" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Processor (CPU)</div>
              <div class="row-sub">Total CPU load percentage and core package temp</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_cpu" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_cpu_freq')">
          <div class="row-left">
            <div class="icon-badge secondary">
              <Icons name="chip" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">CPU Frequency</div>
              <div class="row-sub">Active clock speeds across cluster policies</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_cpu_freq" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_gpu')">
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="gpu" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Graphics (GPU)</div>
              <div class="row-sub">Mali GPU utilization load and temperature</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_gpu" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_gpu_freq')">
          <div class="row-left">
            <div class="icon-badge secondary">
              <Icons name="tune" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">GPU Clock Speed</div>
              <div class="row-sub">Instantaneous Mali devfreq operating frequency</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_gpu_freq" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_ram')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="memory" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">System Memory (RAM)</div>
              <div class="row-sub">Used vs total RAM utilization in GB</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_ram" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_battery')">
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="battery" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Battery &amp; Power</div>
              <div class="row-sub">Wattage (W), current draw (mA), and battery temp</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_battery" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="md3-list-row clickable" @click="toggleMetric('show_net')">
          <div class="row-left">
            <div class="icon-badge secondary">
              <Icons name="rocket" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Network Throughput</div>
              <div class="row-sub">Real-time download and upload transfer rates</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input type="checkbox" v-model="hudStore.config.show_net" @change="hudStore.saveConfigDebounced" />
              <span class="md3-switch-track"><span class="md3-switch-thumb"></span></span>
            </label>
          </div>
        </div>

      </div>

      <!-- Appearance Sliders -->
      <div class="section-title">Appearance &amp; Geometry</div>
      <div class="md3-list-group" style="padding: 16px; display: flex; flex-direction: column; gap: 18px;">
        
        <!-- Scale Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Overlay Scale</span>
            <span class="slider-val">{{ Math.round(hudStore.config.scale * 100) }}%</span>
          </div>
          <input
            type="range"
            min="0.5"
            max="1.3"
            step="0.05"
            class="slider-md3"
            v-model.number="hudStore.config.scale"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Opacity Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Background Opacity</span>
            <span class="slider-val">{{ Math.round(hudStore.config.opacity * 100) }}%</span>
          </div>
          <input
            type="range"
            min="0.1"
            max="1.0"
            step="0.05"
            class="slider-md3"
            v-model.number="hudStore.config.opacity"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Font Size Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Typography Size</span>
            <span class="slider-val">{{ hudStore.config.font_size }} sp</span>
          </div>
          <input
            type="range"
            min="9"
            max="16"
            step="1"
            class="slider-md3"
            v-model.number="hudStore.config.font_size"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Corner Radius Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Corner Radius</span>
            <span class="slider-val">{{ hudStore.config.corner_radius }} dp</span>
          </div>
          <input
            type="range"
            min="4"
            max="24"
            step="2"
            class="slider-md3"
            v-model.number="hudStore.config.corner_radius"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Refresh Interval Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Refresh Interval</span>
            <span class="slider-val">{{ hudStore.config.refresh_interval }} ms</span>
          </div>
          <input
            type="range"
            min="200"
            max="1500"
            step="50"
            class="slider-md3"
            v-model.number="hudStore.config.refresh_interval"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

      </div>

      <!-- Quick Maintenance Tools -->
      <div class="section-title">HUD Management</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="onResetPosition">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="refresh" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Reset Screen Coordinates</div>
              <div class="row-sub">Return floating overlay to default upper-left anchor</div>
            </div>
          </div>
          <Icons name="chevron-right" :size="18" style="color: var(--on-surface-variant);" />
        </div>

        <div class="md3-list-row clickable" @click="onRestartEngine">
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="rocket" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Restart HyperMoon Engine</div>
              <div class="row-sub">Relaunch background telemetry daemon and canvas renderer</div>
            </div>
          </div>
          <Icons name="chevron-right" :size="18" style="color: var(--on-surface-variant);" />
        </div>
      </div>

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 18px 0 28px 0;">
        HyperMoon HUD · Seamless Gaming Overlay
      </div>

    </div>
  </div>
</template>

<script setup>
import { onMounted, onUnmounted, inject } from 'vue'
import { useHyperMoonStore } from '@/stores/hypermoon'
import Icons from '@/components/icons/Icons.vue'

const hudStore = useHyperMoonStore()
const toast = inject('toast')

onMounted(async () => {
  await hudStore.init()
})

onUnmounted(() => {
  hudStore.stopPollingStats()
})

async function onToggleMaster() {
  const target = !hudStore.config.visible
  const msg = await hudStore.toggleMaster(target)
  if (msg && toast) toast(msg)
}

function toggleAutoGaming() {
  hudStore.config.auto_gaming = !hudStore.config.auto_gaming
  hudStore.saveConfigDebounced()
}

function setLayout(isHorizontal) {
  hudStore.config.is_horizontal = isHorizontal
  hudStore.saveConfigDebounced()
}

function setAlignment(align) {
  hudStore.config.align = align
  hudStore.saveConfigDebounced()
}

function toggleMetric(key) {
  hudStore.config[key] = !hudStore.config[key]
  hudStore.saveConfigDebounced()
}

async function onResetPosition() {
  const msg = await hudStore.resetPosition()
  if (msg && toast) toast(msg)
}

async function onRestartEngine() {
  if (toast) toast('Restarting HyperMoon engine...')
  const msg = await hudStore.restartEngine()
  if (msg && toast) toast(msg)
}
</script>

<style scoped>
.btn-icon-back {
  background: var(--surface-container);
  border: 1px solid var(--surface-container-highest);
  color: var(--on-surface);
  width: 36px;
  height: 36px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s ease;
}

.btn-icon-back:active {
  transform: scale(0.92);
  background: var(--surface-container-high);
}

.hud-master-card {
  background: var(--surface-container);
  border: 1px solid var(--surface-container-highest);
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
}

.master-card-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.master-icon-group {
  display: flex;
  align-items: center;
  gap: 12px;
}

.master-card-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface);
  margin-bottom: 2px;
}

.master-card-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.3;
}

.hud-status-chips {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-top: 14px;
  padding-top: 12px;
  border-top: 1px solid var(--surface-container-highest);
}

.chip-item {
  font-size: 10.5px;
  font-variant-numeric: tabular-nums;
  padding: 4px 10px;
  border-radius: 8px;
  background: var(--surface-container-highest);
  color: var(--on-surface-variant);
}

.layout-toggle-container {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.layout-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 10px 12px;
  border-radius: 10px;
  border: 1px solid var(--surface-container-highest);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  font-size: 11.5px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.layout-btn:active {
  transform: scale(0.96);
}

.layout-btn-active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.align-pills {
  display: flex;
  gap: 6px;
}

.align-pill {
  padding: 4px 10px;
  border-radius: 6px;
  border: 1px solid var(--surface-container-highest);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  font-size: 10px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.2s ease;
}

.align-pill-active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.slider-row {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.slider-header {
  display: flex;
  justify-content: space-between;
  font-size: 12px;
}

.slider-label {
  font-weight: 600;
  color: var(--on-surface);
}

.slider-val {
  font-weight: 700;
  font-variant-numeric: tabular-nums;
  color: var(--primary);
}

.slider-md3 {
  -webkit-appearance: none;
  appearance: none;
  width: 100%;
  height: 6px;
  border-radius: 3px;
  background: var(--surface-container-highest);
  outline: none;
}

.slider-md3::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--primary);
  cursor: pointer;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
}

.badge-active {
  background: rgba(34, 197, 94, 0.15) !important;
  color: #22c55e !important;
  border-color: rgba(34, 197, 94, 0.3) !important;
}
</style>
