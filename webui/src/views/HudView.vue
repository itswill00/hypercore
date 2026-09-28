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
      <span class="badge-pill" :class="hudStore.config.visible ? 'badge-active' : ''">
        {{ hudStore.config.visible ? 'Active' : 'Off' }}
      </span>
    </div>

    <div class="content-area">

      <!-- Master Control -->
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge" :class="hudStore.config.visible ? '' : 'secondary'">
              <Icons name="moon" :size="20" />
            </div>
            <div class="row-meta">
              <div class="row-title">Enable Overlay</div>
              <div class="row-sub">
                {{ hudStore.config.visible ? (hudStore.config.auto_gaming ? 'Overlay will automatically show during games' : 'Live stats showing on screen') : 'Overlay is currently off' }}
              </div>
            </div>
          </div>
          <div class="row-val" @click.stop="onToggleMaster">
            <label class="md3-switch" style="pointer-events: none; vertical-align: middle;">
              <input
                type="checkbox"
                :checked="hudStore.config.visible"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>

        <div class="md3-list-row">
          <div class="row-left" style="flex-wrap: wrap; gap: 6px;">
            <span class="badge-pill">Daemon: {{ hudStore.daemonPid ? 'Running' : 'Idle' }}</span>
            <span class="badge-pill">Display: {{ hudStore.overlayPid ? 'Active' : 'Off' }}</span>
            <span class="badge-pill" v-if="hudStore.stats.fps && hudStore.stats.fps !== '--'">{{ hudStore.stats.fps }} FPS</span>
            <span class="badge-pill" v-if="hudStore.stats.screen_hz && hudStore.stats.screen_hz !== '--'">{{ hudStore.stats.screen_hz }}</span>
          </div>
        </div>
      </div>

      <!-- Quick Presets -->
      <div class="section-title">Presets</div>
      <div class="md3-list-group" style="padding: 12px;">
        <div class="preset-buttons">
          <button
            class="preset-btn"
            :class="{ 'preset-btn-active': activePreset === 'compact' }"
            @click="onSelectPreset('compact')"
          >
            <span class="preset-title">Compact</span>
            <span class="preset-desc">FPS, CPU & GPU</span>
          </button>
          <button
            class="preset-btn"
            :class="{ 'preset-btn-active': activePreset === 'minimal' }"
            @click="onSelectPreset('minimal')"
          >
            <span class="preset-title">Minimal</span>
            <span class="preset-desc">FPS Only</span>
          </button>
          <button
            class="preset-btn"
            :class="{ 'preset-btn-active': activePreset === 'detailed' }"
            @click="onSelectPreset('detailed')"
          >
            <span class="preset-title">Detailed</span>
            <span class="preset-desc">Full stats & Govs</span>
          </button>
        </div>
      </div>

      <!-- Layout & Orientation Style -->
      <div class="section-title">Layout</div>
      <div class="md3-list-group" style="padding: 12px;">
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

      <!-- Theme & Accent Color -->
      <div class="section-title">Theme & Color</div>
      <div class="md3-list-group" style="padding: 12px;">
        <div class="theme-grid">
          <button
            v-for="th in themeList"
            :key="th.id"
            class="theme-chip"
            :class="{ 'theme-chip-active': hudStore.config.theme === th.id }"
            @click="onSelectTheme(th.id)"
          >
            <span class="theme-dot" :style="{ backgroundColor: th.color }"></span>
            <span>{{ th.name }}</span>
          </button>
        </div>

        <div v-if="hudStore.config.theme === 'custom'" class="custom-color-row">
          <span style="font-size: 13px; font-weight: 500; color: var(--on-surface-variant);">Custom Accent Color</span>
          <div class="color-picker-wrap">
            <input
              type="color"
              v-model="hudStore.config.custom_color"
              @input="hudStore.saveConfigDebounced"
              @change="hudStore.saveConfig"
              class="color-picker-input"
            />
            <span class="color-hex-val">{{ hudStore.config.custom_color }}</span>
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
          <div class="row-val" @click.stop="toggleItem(item.key)">
            <label class="md3-switch" style="pointer-events: none;">
              <input
                type="checkbox"
                :checked="hudStore.config[item.key]"
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
      <div class="md3-list-group" style="padding: 12px; display: flex; flex-direction: column; gap: 14px;">
        
        <!-- Card Width -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Card Width</span>
            <span class="slider-val">{{ hudStore.config.bg_width || 150 }} px</span>
          </div>
          <input
            type="range"
            min="100"
            max="400"
            step="2"
            class="md3-range-slider"
            v-model.number="hudStore.config.bg_width"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Card Height -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Card Height</span>
            <span class="slider-val">{{ hudStore.config.bg_height || 160 }} px</span>
          </div>
          <input
            type="range"
            min="36"
            max="260"
            step="2"
            class="md3-range-slider"
            v-model.number="hudStore.config.bg_height"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Scale Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Scale</span>
            <span class="slider-val">{{ Math.round((hudStore.config.scale || 0.65) * 100) }}%</span>
          </div>
          <input
            type="range"
            min="0.5"
            max="1.8"
            step="0.05"
            class="md3-range-slider"
            v-model.number="hudStore.config.scale"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Font Size Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Font Size</span>
            <span class="slider-val">{{ hudStore.config.font_size || 14 }} sp</span>
          </div>
          <input
            type="range"
            min="9"
            max="22"
            step="1"
            class="md3-range-slider"
            v-model.number="hudStore.config.font_size"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Corner Radius Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Corner Radius</span>
            <span class="slider-val">{{ hudStore.config.corner_radius ?? 14 }} dp</span>
          </div>
          <input
            type="range"
            min="0"
            max="30"
            step="1"
            class="md3-range-slider"
            v-model.number="hudStore.config.corner_radius"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Refresh Interval Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Update Interval</span>
            <span class="slider-val">{{ hudStore.config.refresh_interval || 1500 }} ms</span>
          </div>
          <input
            type="range"
            min="50"
            max="2500"
            step="50"
            class="md3-range-slider"
            v-model.number="hudStore.config.refresh_interval"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

        <!-- Opacity Slider -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Background Opacity</span>
            <span class="slider-val">{{ Math.round((hudStore.config.opacity ?? 0.70) * 100) }}%</span>
          </div>
          <input
            type="range"
            min="0.1"
            max="1.0"
            step="0.05"
            class="md3-range-slider"
            v-model.number="hudStore.config.opacity"
            @input="hudStore.saveConfigDebounced"
            @change="hudStore.saveConfig"
          />
        </div>

      </div>

      <!-- Overlay Position (works on all Android versions — direct drag is
           unavailable on Android 14 QPR3+/15 where the overlay runs on a
           hardware surface without an input channel) -->
      <div class="section-title">Position</div>
      <div class="md3-list-group" style="padding: 12px; display: flex; flex-direction: column; gap: 12px;">

        <div style="display: flex; gap: 16px; align-items: center;">
          <div class="dpad-grid">
            <span></span>
            <button class="dpad-btn" @click="hudStore.nudgePosition(0, -20)" title="Move up">
              <Icons name="chevron-up" :size="18" />
            </button>
            <span></span>
            <button class="dpad-btn" @click="hudStore.nudgePosition(-20, 0)" title="Move left">
              <Icons name="chevron-left" :size="18" />
            </button>
            <div class="dpad-center">{{ hudStore.position.x }}, {{ hudStore.position.y }}</div>
            <button class="dpad-btn" @click="hudStore.nudgePosition(20, 0)" title="Move right">
              <Icons name="chevron-right" :size="18" />
            </button>
            <span></span>
            <button class="dpad-btn" @click="hudStore.nudgePosition(0, 20)" title="Move down">
              <Icons name="chevron-down" :size="18" />
            </button>
            <span></span>
          </div>
          <div style="flex: 1; font-size: 11px; line-height: 1.5; color: var(--on-surface-variant);">
            Tap arrows to nudge 20&nbsp;px. The overlay clamps itself to screen bounds automatically.
          </div>
        </div>

        <!-- Position X -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Position X</span>
            <span class="slider-val">{{ hudStore.position.x }} px</span>
          </div>
          <input
            type="range"
            min="0"
            max="1440"
            step="5"
            class="md3-range-slider"
            :value="hudStore.position.x"
            @input="onPosX($event)"
          />
        </div>

        <!-- Position Y -->
        <div class="slider-row">
          <div class="slider-header">
            <span class="slider-label">Position Y</span>
            <span class="slider-val">{{ hudStore.position.y }} px</span>
          </div>
          <input
            type="range"
            min="0"
            max="3200"
            step="5"
            class="md3-range-slider"
            :value="hudStore.position.y"
            @input="onPosY($event)"
          />
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
          <div class="row-val" @click.stop="toggleAutoGaming">
            <label class="md3-switch" style="pointer-events: none;">
              <input
                type="checkbox"
                :checked="hudStore.config.auto_gaming"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
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
              <div class="row-sub">Move overlay back to the default spot</div>
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

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 12px 0 20px 0;">
        HyperMoon HUD · In-Game Performance Overlay
      </div>

    </div>
  </div>
</template>

<script setup>
import { onMounted, onUnmounted, computed, inject } from "vue"
import { useHyperMoonStore } from "@/stores/hypermoon"
import Icons from "@/components/icons/Icons.vue"

const hudStore = useHyperMoonStore()
const toast = inject("toast")

const themeList = [
  { id: 'cyber_neon', name: 'Cyber Neon', color: '#38BDF8' },
  { id: 'amoled', name: 'AMOLED Dark', color: '#FFFFFF' },
  { id: 'matrix', name: 'Matrix Green', color: '#22C55E' },
  { id: 'crimson', name: 'Crimson Red', color: '#F43F5E' },
  { id: 'custom', name: 'Custom', color: hudStore.config.custom_color || '#6366F1' }
]

const activePreset = computed(() => {
  const c = hudStore.config
  if (
    c.show_fps &&
    !c.show_cpu &&
    !c.show_cpu_freq &&
    !c.show_gov &&
    !c.show_gpu &&
    !c.show_gpu_freq &&
    !c.show_gpu_gov &&
    !c.show_ram &&
    !c.show_zram &&
    !c.show_battery &&
    !c.show_net
  ) {
    return 'minimal'
  }
  if (
    c.show_fps &&
    c.show_cpu &&
    c.show_cpu_freq &&
    !c.show_gov &&
    c.show_gpu &&
    c.show_gpu_freq &&
    !c.show_gpu_gov &&
    !c.show_ram &&
    !c.show_zram &&
    c.show_battery &&
    !c.show_net
  ) {
    return 'compact'
  }
  if (
    c.show_fps &&
    c.show_cpu &&
    c.show_cpu_freq &&
    c.show_gov &&
    c.show_gpu &&
    c.show_gpu_freq &&
    c.show_gpu_gov &&
    c.show_ram &&
    !c.show_zram &&
    c.show_battery &&
    c.show_net
  ) {
    return 'detailed'
  }
  return null
})

const displayItems = [
  { key: "show_fps", title: "FPS & Refresh Rate", sub: "Frames per second and display Hz", icon: "zap", color: "" },
  { key: "show_cpu", title: "CPU Usage & Temp", sub: "Total processor load and temperature", icon: "cpu", color: "" },
  { key: "show_cpu_freq", title: "CPU Clock Speed", sub: "Live operating frequency across cores", icon: "chip", color: "secondary" },
  { key: "show_gov", title: "CPU Governor", sub: "Scaling governor and policy range", icon: "tune", color: "" },
  { key: "show_gpu", title: "GPU Usage & Temp", sub: "Mali graphics load and temperature", icon: "gpu", color: "tertiary" },
  { key: "show_gpu_freq", title: "GPU Clock Speed", sub: "Active graphics processor frequency", icon: "zap", color: "secondary" },
  { key: "show_gpu_gov", title: "GPU Governor", sub: "Active graphics governor and policy", icon: "sliders", color: "tertiary" },
  { key: "show_ram", title: "Memory (RAM)", sub: "Used and total system memory", icon: "memory", color: "" },
  { key: "show_zram", title: "ZRAM Swap", sub: "Compressed memory swap usage", icon: "chip", color: "secondary" },
  { key: "show_battery", title: "Battery & Power", sub: "Current wattage, charge rate, and temperature", icon: "battery", color: "tertiary" },
  { key: "show_net", title: "Network Speed", sub: "Real-time download and upload speeds", icon: "rocket", color: "secondary" }
]

onMounted(async () => {
  await hudStore.init()
})

onUnmounted(() => {
  hudStore.stopPollingStats()
})

function onToggleMaster() {
  hudStore.toggleMaster(!hudStore.config.visible)
}

function onSelectPreset(type) {
  hudStore.applyPreset(type)
  const label = type.charAt(0).toUpperCase() + type.slice(1)
  if (toast) toast(`${label} preset applied`)
}

function onSelectTheme(themeId) {
  hudStore.setTheme(themeId)
}

function toggleAutoGaming() {
  hudStore.config.auto_gaming = !hudStore.config.auto_gaming
  if (hudStore.config.auto_gaming && !hudStore.config.visible) {
    hudStore.config.visible = true
  }
  hudStore.saveConfigDebounced()
  if (toast) {
    toast(hudStore.config.auto_gaming ? 'Enabled: Overlay will show only in games' : 'Disabled: Overlay follows master switch')
  }
}

function setLayout(isHorizontal) {
  hudStore.setLayout(isHorizontal)
}

function setAlignment(align) {
  hudStore.setAlignment(align)
}

function toggleItem(key) {
  hudStore.config[key] = !hudStore.config[key]
  hudStore.saveConfigDebounced()
}

async function onResetPosition() {
  const msg = await hudStore.resetPosition()
  if (msg && toast) toast(msg)
}

function onPosX(e) {
  hudStore.setPosition(+e.target.value, hudStore.position.y)
}

function onPosY(e) {
  hudStore.setPosition(hudStore.position.x, +e.target.value)
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
  padding: 8px 10px;
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

.dpad-grid {
  display: grid;
  grid-template-columns: repeat(3, 36px);
  grid-template-rows: repeat(3, 36px);
  gap: 4px;
  flex-shrink: 0;
}

.dpad-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 10px;
  border: 1px solid var(--surface-container-high);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: all 0.15s ease;
  -webkit-tap-highlight-color: transparent;
}

.dpad-btn:active {
  transform: scale(0.92);
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.dpad-center {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 9px;
  font-weight: 700;
  font-variant-numeric: tabular-nums;
  color: var(--on-surface-variant);
  text-align: center;
  line-height: 1.2;
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

.preset-buttons {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
}

.preset-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 8px 4px;
  border-radius: 12px;
  border: 1px solid var(--surface-container-high);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: all 0.2s ease;
  text-align: center;
  gap: 2px;
  -webkit-tap-highlight-color: transparent;
}

.preset-btn:active {
  transform: scale(0.96);
}

.preset-btn-active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.preset-title {
  font-size: 12px;
  font-weight: 700;
}

.preset-desc {
  font-size: 10px;
  opacity: 0.8;
}

.preset-btn-active .preset-desc {
  opacity: 0.9;
}

.theme-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.theme-chip {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 10px;
  border-radius: 10px;
  border: 1px solid var(--surface-container-high);
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  font-size: 11.5px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
  -webkit-tap-highlight-color: transparent;
}

.theme-chip:active {
  transform: scale(0.96);
}

.theme-chip-active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.theme-dot {
  width: 10px;
  height: 10px;
  border-radius: 3px;
  flex-shrink: 0;
  border: 1px solid rgba(255, 255, 255, 0.15);
}

.custom-color-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px solid var(--surface-container-high);
}

.color-picker-wrap {
  display: flex;
  align-items: center;
  gap: 8px;
}

.color-picker-input {
  -webkit-appearance: none;
  -moz-appearance: none;
  appearance: none;
  width: 32px;
  height: 32px;
  background: none;
  border: none;
  cursor: pointer;
  border-radius: 8px;
  overflow: hidden;
}

.color-picker-input::-webkit-color-swatch-wrapper {
  padding: 0;
}

.color-picker-input::-webkit-color-swatch {
  border: 1px solid var(--outline-variant);
  border-radius: 8px;
}

.color-hex-val {
  font-size: 12px;
  font-weight: 600;
  font-variant-numeric: tabular-nums;
  color: var(--on-surface);
  background: var(--surface-container-highest);
  padding: 4px 8px;
  border-radius: 6px;
}
</style>
