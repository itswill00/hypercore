<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">Charger Control</div>
        <div class="page-header-sub">Hardware charging speed &amp; battery protection</div>
      </div>
      <span class="badge-pill" style="font-size: 11px; padding: 4px 10px;" :style="!store.chargerSupported ? 'background: var(--surface-container-highest); color: var(--on-surface-variant);' : ''">
        {{ activeModeName }}
      </span>
    </div>

    <div class="content-area">

      <!-- Device Unsupported Warning Banner -->
      <div v-if="!store.chargerSupported" class="md3-banner" style="background: var(--surface-container-high); border-color: var(--outline-variant); margin-bottom: 14px;">
        <div style="display: flex; align-items: flex-start; gap: 12px;">
          <div class="icon-badge secondary">
            <Icons name="shield" :size="20" />
          </div>
          <div>
            <div style="font-size: 13px; font-weight: 700; color: var(--on-surface); margin-bottom: 2px;">Your device does not support this function properly</div>
            <div style="font-size: 11px; color: var(--on-surface-variant); line-height: 1.4;">
              Required hardware charger nodes (/sys/class/power_supply/battery/charge_control_limit) were not detected on this kernel interface. Control features have been safely disabled.
            </div>
          </div>
        </div>
      </div>

      <!-- Thermal Override Alert Banner -->
      <div v-else-if="store.chargeModeOverride" class="md3-banner" style="background: var(--error-container); color: var(--on-error-container); border-color: var(--error); margin-bottom: 14px;">
        <div style="display: flex; align-items: flex-start; gap: 12px;">
          <div class="icon-badge" style="background: rgba(0,0,0,0.2); color: inherit;">
            <Icons name="thermal" :size="20" />
          </div>
          <div>
            <div style="font-size: 13px; font-weight: 700; margin-bottom: 2px;">Thermal Safety Override</div>
            <div style="font-size: 11px; opacity: 0.9; line-height: 1.4;">
              Daemon is gradually stepping down charging speed to control temperature ({{ store.batTemp }}°C). Full speed will automatically restore once temperature stabilizes below 41°C.
            </div>
          </div>
        </div>
      </div>

      <!-- Non-Original Cable / Power Cap Alert Banner -->
      <div v-if="store.isNonOemCable" class="md3-banner" style="background: var(--surface-container-high); border-color: var(--outline-variant); margin-bottom: 14px;">
        <div style="display: flex; align-items: flex-start; gap: 12px;">
          <div class="icon-badge secondary">
            <Icons name="zap" :size="20" />
          </div>
          <div>
            <div style="font-size: 13px; font-weight: 700; color: var(--on-surface); margin-bottom: 2px;">Standard Cable Hardware Ceiling</div>
            <div style="font-size: 11px; color: var(--on-surface-variant); line-height: 1.4;">
              Connected cable/charger is hardware-capped to ~6W max by the PMIC. Switch to an official 33W OEM cable to unlock peak charging speeds.
            </div>
          </div>
        </div>
      </div>

      <!-- Power & Battery Status Group -->
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="zap" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Power &amp; Battery State</div>
              <div class="row-sub">Current flow, voltage &amp; battery stats</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill" :style="store.batStatus === 'Charging' ? 'background: var(--primary-container); color: var(--on-primary-container);' : ''">
              {{ store.batStatus }} {{ store.batLevel ? `(${store.batLevel}%)` : '' }}
            </span>
          </div>
        </div>

        <div class="md3-list-row">
          <div class="row-left" style="width: 100%;">
            <div class="stat-grid-2">
              <div class="stat-box">
                <div class="stat-lbl">Input Current</div>
                <div class="stat-num">{{ displayCurrentMa }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Input Voltage</div>
                <div class="stat-num">{{ displayVoltage }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Estimated Power</div>
                <div class="stat-num">{{ displayWatt }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Battery Temp</div>
                <div class="stat-num" :style="store.batTemp >= 45 ? 'color: var(--error); font-weight: 700;' : ''">{{ store.batTemp }}°C</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Battery Health</div>
                <div class="stat-num">{{ store.batHealth || 'Good' }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Charge Cycles</div>
                <div class="stat-num">{{ store.batteryCycles ? `${store.batteryCycles} cycles` : '—' }}</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Section: Charging Speed (Precision Slider) -->
      <div class="section-header-row">
        <div class="section-title" style="margin-bottom: 0;">Charging Speed</div>
        <span class="section-badge">{{ currentStep.tag }} · Level {{ currentStep.limit }}</span>
      </div>

      <div class="md3-card mb-3 slider-card">
        <!-- Card Top Bar: Target Status -->
        <div class="slider-top-header">
          <div class="slider-title-meta">
            <div class="slider-main-title">{{ currentStep.label }}</div>
            <div class="slider-sub-meta">
              <span class="step-tag" :style="{ background: currentStep.bgColor, color: currentStep.color }">
                {{ currentStep.tag }}
              </span>
              <span>HW Level: {{ currentStep.limit }}</span>
            </div>
          </div>
          <div class="slider-rate-badge" :style="{ borderColor: currentStep.color }">
            <div class="slider-badge-val" :style="{ color: currentStep.color }">{{ currentStep.current }}</div>
            <div class="slider-badge-pow">{{ currentStep.power }}</div>
          </div>
        </div>

        <!-- Custom MD3 Range Slider with Precision Stepper -->
        <div class="slider-track-box">
          <div class="slider-control-row">
            <button
              class="step-arrow-btn"
              :disabled="sliderValue <= 0 || !store.chargerSupported"
              @click="stepDown"
              title="Step down"
            >
              −
            </button>
            <div class="slider-input-container">
              <input
                type="range"
                min="0"
                :max="sliderSteps.length - 1"
                step="1"
                v-model.number="sliderValue"
                @input="onSliderInput"
                class="md3-range-slider"
                :disabled="!store.chargerSupported"
              />
            </div>
            <button
              class="step-arrow-btn"
              :disabled="sliderValue >= sliderSteps.length - 1 || !store.chargerSupported"
              @click="stepUp"
              title="Step up"
            >
              +
            </button>
          </div>
          <div class="slider-axis-labels">
            <span>Slow (~0.5A)</span>
            <span>Balanced (~2.3A)</span>
            <span>Turbo (~4.5A)</span>
          </div>
        </div>

        <!-- Quick Jump Preset Chips -->
        <div class="slider-quick-row">
          <button
            v-for="(pill, pIdx) in quickPills"
            :key="pIdx"
            class="quick-pill-btn"
            :class="{ 'is-selected': currentStepIndex === pill.step }"
            @click="setSliderStep(pill.step)"
            :disabled="!store.chargerSupported"
          >
            <span>{{ pill.name }}</span>
            <span class="pill-amp">{{ pill.rate }}</span>
          </button>
        </div>
      </div>

      <!-- Section: Hardware Battery Protection -->
      <div class="section-header-row">
        <div class="section-title" style="margin-bottom: 0;">Battery Protection</div>
      </div>

      <div class="md3-list-group">
        <!-- Night Charging Protection Row -->
        <div
          class="md3-list-row clickable"
          :class="{ 'disabled-mode-row': !store.chargerSupported }"
          @click="toggleNightCharging"
        >
          <div class="row-left">
            <div class="icon-badge tertiary">
              <Icons name="moon" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Night Charging Protection</div>
              <div class="row-sub">Pauses at 80% overnight to prevent sustained high voltage stress</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                :checked="store.nightCharging"
                @change="toggleNightCharging"
                :disabled="!store.chargerSupported"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>

        <!-- Smart Charging Curve Row -->
        <div
          class="md3-list-row clickable"
          :class="{ 'disabled-mode-row': !store.chargerSupported }"
          @click="toggleSmartChg"
        >
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="chip" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Smart Charging Curve</div>
              <div class="row-sub">Dynamic thermal regulation and adaptive current throttling</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                :checked="store.smartChg"
                @change="toggleSmartChg"
                :disabled="!store.chargerSupported"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>

        <!-- Limit Charging to 80% Row -->
        <div
          class="md3-list-row clickable"
          :class="{ 'disabled-mode-row': !store.chargerSupported }"
          @click="toggleProtect80"
        >
          <div class="row-left">
            <div class="icon-badge secondary">
              <Icons name="shield" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Limit Charging to 80%</div>
              <div class="row-sub">Hard stop charging at 80% capacity to extend battery cycle life</div>
            </div>
          </div>
          <div class="row-val" @click.stop>
            <label class="md3-switch">
              <input
                type="checkbox"
                :checked="store.protect80"
                @change="toggleProtect80"
                :disabled="!store.chargerSupported"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>
      </div>

    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch, inject, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')
const sliderSteps = [
  { step: 0,  limit: 15, current: '~500 mA',  power: '~2.0 W',  label: 'Trickle Minimum',   tag: 'Safe',     color: 'var(--tertiary)', bgColor: 'rgba(125, 205, 255, 0.12)' },
  { step: 1,  limit: 14, current: '~750 mA',  power: '~2.8 W',  label: 'Gentle Overnight',  tag: 'Safe',     color: 'var(--tertiary)', bgColor: 'rgba(125, 205, 255, 0.12)' },
  { step: 2,  limit: 13, current: '~1,200 mA', power: '~4.7 W', label: 'Cool Steady',       tag: 'Gentle',   color: 'var(--primary)',  bgColor: 'rgba(120, 180, 255, 0.12)' },
  { step: 3,  limit: 12, current: '~1,600 mA', power: '~6.3 W', label: 'Mild Steady',       tag: 'Cool',     color: 'var(--primary)',  bgColor: 'rgba(120, 180, 255, 0.12)' },
  { step: 4,  limit: 11, current: '~2,000 mA', power: '~8.0 W', label: 'Daily Efficient',   tag: 'Normal',   color: 'var(--primary)',  bgColor: 'rgba(120, 180, 255, 0.12)' },
  { step: 5,  limit: 10, current: '~2,300 mA', power: '~9.3 W', label: 'Balanced Sweetspot',tag: 'Balanced', color: 'var(--primary)', bgColor: 'rgba(120, 180, 255, 0.12)' },
  { step: 6,  limit: 9,  current: '~2,550 mA', power: '~10.3 W', label: 'Balanced Boost',   tag: 'Warm',     color: '#ffb74d',         bgColor: 'rgba(255, 183, 77, 0.14)' },
  { step: 7,  limit: 8,  current: '~2,800 mA', power: '~11.3 W', label: 'Express Flow',     tag: 'Warm',     color: '#ffb74d',         bgColor: 'rgba(255, 183, 77, 0.14)' },
  { step: 8,  limit: 7,  current: '~3,000 mA', power: '~12.2 W', label: 'Express Boost',    tag: 'Fast',     color: '#ffa726',         bgColor: 'rgba(255, 167, 38, 0.14)' },
  { step: 9,  limit: 6,  current: '~3,200 mA', power: '~13.1 W', label: 'High Speed',       tag: 'Fast',     color: '#ffa726',         bgColor: 'rgba(255, 167, 38, 0.14)' },
  { step: 10, limit: 5,  current: '~3,450 mA', power: '~14.1 W', label: 'Turbo Flow',       tag: 'Fast+',    color: '#ff9800',         bgColor: 'rgba(255, 152, 0, 0.14)' },
  { step: 11, limit: 4,  current: '~3,700 mA', power: '~15.2 W', label: 'Turbo Rate',       tag: 'Hot',      color: '#ff7043',         bgColor: 'rgba(255, 112, 67, 0.14)' },
  { step: 12, limit: 3,  current: '~3,900 mA', power: '~16.0 W', label: 'Peak Flow',        tag: 'Hot+',     color: '#ff5722',         bgColor: 'rgba(255, 87, 34, 0.14)' },
  { step: 13, limit: 2,  current: '~4,100 mA', power: '~16.9 W', label: 'Extreme Rate',     tag: 'Peak',     color: '#f4511e',         bgColor: 'rgba(244, 81, 30, 0.14)' },
  { step: 14, limit: 1,  current: '~4,300 mA', power: '~17.7 W', label: 'Ultra Peak',       tag: 'Peak',     color: '#e53935',         bgColor: 'rgba(229, 57, 53, 0.15)' },
  { step: 15, limit: 0,  current: '~4,500 mA', power: '~18.5 W+',label: 'Max Unrestricted', tag: 'Max',      color: '#d32f2f',         bgColor: 'rgba(211, 47, 47, 0.15)' }
]

const quickPills = [
  { name: 'Trickle', rate: '0.5A', step: 0 },
  { name: 'Cool',    rate: '1.6A', step: 3 },
  { name: 'Balanced',rate: '2.3A', step: 5 },
  { name: 'High',    rate: '3.2A', step: 9 },
  { name: 'Max',     rate: '4.5A', step: 15 }
]

const currentStepIndex = computed(() => {
  const lim = store.customLimit
  const foundIdx = sliderSteps.findIndex(s => s.limit === lim)
  return foundIdx !== -1 ? foundIdx : 5
})

const sliderValue = ref(currentStepIndex.value)

watch(() => store.customLimit, (newLim) => {
  const idx = sliderSteps.findIndex(s => s.limit === newLim)
  if (idx !== -1) sliderValue.value = idx
})

const currentStep = computed(() => {
  return sliderSteps[sliderValue.value] || sliderSteps[5]
})

let sliderDebounceTimer = null
function onSliderInput() {
  const stepObj = sliderSteps[sliderValue.value]
  if (!stepObj) return
  
  clearTimeout(sliderDebounceTimer)
  sliderDebounceTimer = setTimeout(async () => {
    await store.setCustomLimit(stepObj.limit)
    if (toast) toast(`Charging target: Level ${stepObj.limit} · ${stepObj.current}`)
  }, 180)
}

async function setSliderStep(stepIdx) {
  if (stepIdx < 0 || stepIdx >= sliderSteps.length) return
  sliderValue.value = stepIdx
  const stepObj = sliderSteps[stepIdx]
  if (!stepObj) return
  await store.setCustomLimit(stepObj.limit)
  if (toast) toast(`Charging target: Level ${stepObj.limit} · ${stepObj.current}`)
}

function stepDown() {
  if (sliderValue.value > 0) {
    setSliderStep(sliderValue.value - 1)
  }
}

function stepUp() {
  if (sliderValue.value < sliderSteps.length - 1) {
    setSliderStep(sliderValue.value + 1)
  }
}

async function toggleNightCharging() {
  if (!store.chargerSupported) return
  const next = !store.nightCharging
  await store.setNightCharging(next)
  if (toast) toast(`Night Charging ${next ? 'enabled (pauses at 80% overnight)' : 'disabled (charges to 100%)'}`)
}

async function toggleSmartChg() {
  if (!store.chargerSupported) return
  const next = !store.smartChg
  await store.setSmartChg(next)
  if (toast) toast(`Smart Charging curve ${next ? 'enabled' : 'disabled'}`)
}

async function toggleProtect80() {
  if (!store.chargerSupported) return
  const next = !store.protect80
  await store.setProtect80(next)
  if (toast) toast(`80% Battery Limit ${next ? 'enabled (stops at 80%)' : 'disabled (unrestricted to 100%)'}`)
}

const activeModeName = computed(() => {
  if (!store.chargerSupported) return 'Unsupported'
  return `${currentStep.value.label} (${currentStep.value.current})`
})

const displayCurrentMa = computed(() => {
  const ma = store.chargeCurrentMa
  if (!ma || ma === 0) return store.batStatus === 'Charging' ? '— mA' : '0 mA'
  const sign = store.batStatus === 'Charging' ? '+' : '-'
  return `${sign}${ma.toLocaleString()} mA`
})

const displayVoltage = computed(() => {
  const mv = store.chargeVoltMv
  if (!mv || mv === 0) return '—'
  return `${(mv / 1000).toFixed(2)} V`
})

const displayWatt = computed(() => {
  const ma = store.chargeCurrentMa
  const mv = store.chargeVoltMv
  if (!ma || !mv || ma === 0) return '0 W'
  return `${((ma * mv) / 1_000_000).toFixed(1)} W`
})

onMounted(() => {
  store.pollCharger()
  store.startCardPolling()
})

onUnmounted(() => {
  store.stopCardPolling()
})
</script>

<style scoped>
.section-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin: 18px 0 10px 0;
}

.section-badge {
  font-size: 10px;
  font-weight: 700;
  color: var(--primary);
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  padding: 3px 8px;
  border-radius: 6px;
  letter-spacing: 0.2px;
  font-variant-numeric: tabular-nums;
}

.slider-card {
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  background: var(--surface-container-low);
  border: 1px solid var(--surface-container-highest);
  border-radius: 16px;
}

.slider-top-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.slider-title-meta {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.slider-main-title {
  font-size: 16px;
  font-weight: 700;
  color: var(--on-surface);
  letter-spacing: -0.2px;
}

.slider-sub-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 11px;
  color: var(--on-surface-variant);
  font-family: inherit;
  font-variant-numeric: tabular-nums;
}

.step-tag {
  font-size: 9.5px;
  font-weight: 700;
  padding: 2px 7px;
  border-radius: 6px;
  letter-spacing: 0.3px;
  text-transform: uppercase;
}

.slider-rate-badge {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  padding: 4px 10px;
  border-radius: 10px;
  border: 1px solid var(--primary);
  background: var(--surface-container);
}

.slider-badge-val {
  font-size: 15px;
  font-weight: 800;
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  letter-spacing: -0.2px;
}

.slider-badge-pow {
  font-size: 10.5px;
  color: var(--on-surface-variant);
  font-family: inherit;
  font-variant-numeric: tabular-nums;
}

.slider-track-box {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 4px 0;
}

.slider-input-container {
  width: 100%;
}

.slider-control-row {
  display: flex;
  align-items: center;
  gap: 10px;
  width: 100%;
}

.step-arrow-btn {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 50%;
  color: var(--on-surface);
  font-size: 18px;
  font-weight: 600;
  cursor: pointer;
  flex-shrink: 0;
  transition: transform 0.22s cubic-bezier(0.34, 1.56, 0.64, 1), background 0.15s ease, color 0.15s ease, border-color 0.15s ease;
  user-select: none;
  -webkit-tap-highlight-color: transparent;
}

.step-arrow-btn:active:not(:disabled) {
  background: var(--primary-container);
  color: var(--on-primary-container);
  transform: scale(0.88);
  transition: transform 0.08s ease;
}

.step-arrow-btn:disabled {
  opacity: 0.3;
  cursor: not-allowed;
}

.md3-range-slider {
  -webkit-appearance: none;
  width: 100%;
  height: 8px;
  border-radius: 4px;
  background: var(--surface-container-highest);
  outline: none;
  accent-color: var(--primary);
}

.md3-range-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: var(--primary);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.35);
  cursor: pointer;
  border: 2px solid var(--surface);
  transition: transform 0.22s cubic-bezier(0.34, 1.56, 0.64, 1), box-shadow 0.2s ease;
}

.md3-range-slider::-webkit-slider-thumb:active {
  transform: scale(1.22);
  box-shadow: 0 0 0 8px rgba(200, 198, 215, 0.18), 0 4px 10px rgba(0, 0, 0, 0.4);
}

.slider-axis-labels {
  display: flex;
  justify-content: space-between;
  font-size: 10px;
  color: var(--on-surface-variant);
  opacity: 0.7;
  font-family: inherit;
  font-variant-numeric: tabular-nums;
}

.slider-quick-row {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 6px;
}

.quick-pill-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 7px 4px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 10px;
  color: var(--on-surface);
  cursor: pointer;
  transition: transform 0.22s cubic-bezier(0.34, 1.56, 0.64, 1), background 0.18s ease, border-color 0.18s ease, color 0.18s ease, box-shadow 0.2s ease;
  user-select: none;
  -webkit-tap-highlight-color: transparent;
}

.quick-pill-btn span {
  font-size: 11px;
  font-weight: 600;
}

.quick-pill-btn .pill-amp {
  font-size: 9.5px;
  color: var(--on-surface-variant);
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  margin-top: 2px;
}

.quick-pill-btn:active {
  transform: scale(0.92);
  transition: transform 0.08s ease;
}

.quick-pill-btn.is-selected {
  background: var(--primary-container);
  border-color: var(--primary);
  color: var(--on-primary-container);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.25);
  transform: scale(1.02);
}

.quick-pill-btn.is-selected .pill-amp {
  color: var(--on-primary-container);
  opacity: 0.85;
}

.disabled-mode-row {
  opacity: 0.45;
  cursor: not-allowed !important;
}

/* Material Design 3 Precision Switch */
.md3-switch {
  position: relative;
  display: inline-block;
  width: 46px;
  height: 26px;
  flex-shrink: 0;
  cursor: pointer;
  vertical-align: middle;
  -webkit-tap-highlight-color: transparent;
}

.md3-switch input {
  position: absolute;
  opacity: 0;
  width: 0;
  height: 0;
}

.md3-switch-track {
  position: absolute;
  inset: 0;
  background: var(--surface-container-highest);
  border: 1.5px solid var(--outline);
  border-radius: 26px;
  transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  box-sizing: border-box;
}

.md3-switch-thumb {
  position: absolute;
  width: 14px;
  height: 14px;
  left: 4px;
  top: 4px;
  border-radius: 50%;
  background: var(--outline);
  transition: transform 0.22s cubic-bezier(0.34, 1.56, 0.64, 1),
              width 0.18s cubic-bezier(0.34, 1.56, 0.64, 1),
              height 0.18s cubic-bezier(0.34, 1.56, 0.64, 1),
              left 0.22s cubic-bezier(0.34, 1.56, 0.64, 1),
              top 0.22s cubic-bezier(0.34, 1.56, 0.64, 1),
              background 0.2s ease;
  box-sizing: border-box;
}

.md3-switch:active .md3-switch-thumb {
  width: 17px;
}

.md3-switch input:checked + .md3-switch-track {
  background: var(--primary);
  border-color: var(--primary);
}

.md3-switch input:checked + .md3-switch-track .md3-switch-thumb {
  transform: translateX(18px);
  width: 18px;
  height: 18px;
  left: 3px;
  top: 2px;
  background: var(--on-primary);
}

.md3-switch input:checked:active + .md3-switch-track .md3-switch-thumb {
  width: 21px;
  transform: translateX(15px);
}

.md3-switch input:disabled + .md3-switch-track {
  opacity: 0.38;
  cursor: not-allowed;
}
</style>
