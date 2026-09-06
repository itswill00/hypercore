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
          <div class="page-header-sub">Live on-screen performance HUD</div>
        </div>
      </div>
      <span class="badge-pill" :class="hudStore.isRunning ? 'badge-active' : ''">
        {{ hudStore.isRunning ? 'Active' : 'Off' }}
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
              <div class="master-card-title">Enable Overlay</div>
              <div class="master-card-sub">
                {{ hudStore.config.visible ? 'Live stats showing on screen' : 'Overlay is currently off' }}
              </div>
            </div>
          </div>
          <label class="md3-switch" @click.stop>
            <input
              type="checkbox"
              :checked="hudStore.config.visible"
              @change="onToggleMaster"
            />
            <span class="md3-switch-track">
              <span class="md3-switch-thumb"></span>
            </span>
          </label>
        </div>

        <div class="hud-status-chips">
          <div class="chip-item">
            <span>Daemon: {{ hudStore.daemonPid ? 'Running' : 'Idle' }}</span>
          </div>
          <div class="chip-item">
            <span>Display: {{ hudStore.overlayPid ? 'Active' : 'Off' }}</span>
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
      <div class="section-title">Automation</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="toggleAutoGaming">
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="games" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Show only in games</div>
              <div class="row-sub">Automatically show overlay when a game starts and hide when you exit</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                :checked="hudStore.config.auto_gaming"
                @change="toggleAutoGaming"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>
      </div>

      <!-- Layout & Orientation Style -->
      <div class="section-title">Layout</div>
      <div class="md3-list-group" style="padding: 14px;">
        <div class="layout-toggle-container">
          <button
            class="layout-btn"
            :class="{ 'layout-btn-active': hudStore.config.is_horizontal }"
            @click="setLayout(true)"
          >
            <Icons name="more-vertical" :size="16" style="transform: rotate(90deg);" />
            <span>Horizontal Bar</span>
          </button>
          <button
            class="layout-btn"
            :class="{ 'layout-btn-active': !hudStore.config.is_horizontal }"
            @click="setLayout(false)"
          >
            <Icons name="grid" :size="16" />
            <span>Vertical Card</span>
          </button>
        </div>

        <div style="display: flex; align-items: center; justify-content: space-between; margin-top: 14px; padding-top: 12px; border-top: 1px solid var(--surface-container-high);">
          <span style="font-size: 13px; font-weight: 500; color: var(--on-surface-variant);">Text Align</span>
          <div class="align-pills">
            <button
              v-for="a in ['left', 'center', 'right']"
              :key="a"
              class="align-pill"
              :class="{ 'align-pill-active': hudStore.config.align === a }"
              @click="setAlignment(a)"
            >
              {{ a.charAt(0).toUpperCase() + a.slice(1) }}
            </button>
          </div>
        </div>
      </div>

      <!-- What to Show Toggles -->
      <div class="section-title">What to Show</div>
      <div class="md3-list-group">
        <div
          v-for="item in displayItems"
          :key="item.key"
          class="md3-list-row clickable"
          @click="toggleItem(item.key)"
        >
          <div class="row-left">
            <div class="icon-badge" :class="item.color">
              <Icons :name="item.icon" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">{{ item.title }}</div>
              <div class="row-sub">{{ item.sub }}</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                :checked="hudStore.config[item.key]"
                @change="toggleItem(item.key)"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>
      </div>

      <!-- Appearance Sliders -->
      <div class="section-title">Appearance</div>
      <div class="md3-list-group" style="padding: 16px; display: flex; flex-direction: column; gap: 18px;">
        
        <!-- Scale Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Scale</span>
            <span class="slider-val">{{ Math.round(hudStore.config.scale * 100) }}%</span>
          </div>
          <input
            type="range"
            min="0.5"
            max="1.3"
            step="0.05"
            class="md3-range-slider"
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
            class="md3-range-slider"
            v-model.number="hudStore.config.opacity"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Font Size Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Font Size</span>
            <span class="slider-val">{{ hudStore.config.font_size }} sp</span>
          </div>
          <input
            type="range"
            min="9"
            max="16"
            step="1"
            class="md3-range-slider"
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
            class="md3-range-slider"
            v-model.number="hudStore.config.corner_radius"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

        <!-- Refresh Interval Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Update Interval</span>
            <span class="slider-val">{{ hudStore.config.refresh_interval }} ms</span>
          </div>
          <input
            type="range"
            min="200"
            max="1500"
            step="50"
            class="md3-range-slider"
            v-model.number="hudStore.config.refresh_interval"
            @input="hudStore.saveConfigDebounced"
          />
        </div>

      </div>

      <!-- Quick Maintenance Tools -->
      <div class="section-title">Actions</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="onResetPosition">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="refresh" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Reset Position</div>
              <div class="row-sub">Move overlay back to top-left corner</div>
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
              <div class="row-title">Restart HUD</div>
              <div class="row-sub">Restart background service and overlay display</div>
            </div>
          </div>
          <Icons name="chevron-right" :size="18" style="color: var(--on-surface-variant);" />
        </div>
      </div>

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 18px 0 28px 0;">
        HyperMoon HUD · In-Game Performance Overlay
      </div>

    </div>
  </div>
</template>

<script setup>
import { onMounted, onUnmounted, inject } from "vue"
import { useHyperMoonStore } from "@/stores/hypermoon"
import Icons from "@/components/icons/Icons.vue"

const hudStore = useHyperMoonStore()
const toast = inject("toast")

const displayItems = [
  { key: "show_fps", title: "FPS & Refresh Rate", sub: "Frames per second and display Hz", icon: "zap", color: "" },
  { key: "show_cpu", title: "CPU Usage & Temp", sub: "Total processor load and temperature", icon: "cpu", color: "" },
  { key: "show_cpu_freq", title: "CPU Clock Speed", sub: "Live clock frequency across cores", icon: "chip", color: "secondary" },
  { key: "show_gpu", title: "GPU Usage & Temp", sub: "Mali graphics load and temperature", icon: "gpu", color: "tertiary" },
  { key: "show_gpu_freq", title: "GPU Clock Speed", sub: "Mali operating frequency", icon: "tune", color: "secondary" },
  { key: "show_ram", title: "Memory (RAM)", sub: "Used and total system memory", icon: "memory", color: "" },
  { key: "show_battery", title: "Battery & Power", sub: "Current wattage, charge rate, and temperature", icon: "battery", color: "tertiary" },
  { key: "show_net", title: "Network Speed", sub: "Real-time download and upload speeds", icon: "rocket", color: "secondary" }
]

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

function toggleItem(key) {
  hudStore.config[key] = !hudStore.config[key]
  hudStore.saveConfigDebounced()
}

async function onResetPosition() {
  const msg = await hudStore.resetPosition()
  if (msg && toast) toast(msg)
}

async function onRestartEngine() {
  if (toast) toast("Restarting HUD...")
  const msg = await hudStore.restartEngine()
  if (msg && toast) toast(msg)
}
</script>

<style scoped>
.btn-icon-back {
  background: var(--surface-container);
  border: 1px solid var(--surface-container-high);
  color: var(--on-surface);
  width: 34px;
  height: 34px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.btn-icon-back:active {
  transform: scale(0.92);
  background: var(--surface-container-highest);
}

.hud-master-card {
  background: var(--surface-container);
  border: 1px solid var(--surface-container-high);
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 14px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
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
  border-top: 1px solid var(--surface-container-high);
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
  border: 1px solid var(--surface-container-high);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.layout-btn:active {
  transform: scale(0.97);
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
  padding: 4px 12px;
  border-radius: 8px;
  border: 1px solid var(--surface-container-high);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.align-pill:active {
  transform: scale(0.95);
}

.align-pill-active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.slider-row {
  display: flex;
  flex-direction: column;
  gap: 8px;
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

.clickable {
  cursor: pointer;
}

.clickable:active {
  background: var(--surface-container-high);
}

.badge-active {
  background: var(--primary) !important;
  color: var(--on-primary) !important;
  border-color: var(--primary) !important;
  font-weight: 700 !important;
}
</style>
