<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">Charger Control</div>
        <div class="page-header-sub">Charging speed, thermal guard &amp; bypass mode</div>
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
              Daemon temporarily reduced charging speed to Safe mode due to high battery temperature ({{ store.batTemp }}°C). Mode will automatically restore once temperature drops below 40°C.
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
              <div class="row-sub">{{ activePowerSummary }}</div>
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

      <!-- Charger Mode Selection Group -->
      <div class="section-title">Select Charging Mode</div>
      <div class="md3-list-group">

        <div
          v-for="m in modes"
          :key="m.id"
          class="md3-list-row expandable-row clickable"
          :class="{
            'is-expanded': expandedRows[m.id],
            'selected-mode': store.chargerSupported && store.chargeMode === m.id,
            'violent-mode-row': m.id === 5 && store.chargeMode === 5 && store.chargerSupported,
            'disabled-mode-row': !store.chargerSupported
          }"
          @click="onSelectMode(m.id)"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge" :class="m.badgeClass">
                <Icons :name="m.icon" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">{{ m.name }}</div>
                <div class="row-sub">{{ m.estCurrent }} ({{ m.estPower }})</div>
              </div>
            </div>
            <div class="row-val">
              <span v-if="!store.chargerSupported" class="badge-pill">
                Unsupported
              </span>
              <span v-else-if="store.chargeMode === m.id" class="badge-pill" :style="m.id === 5 ? 'background: var(--error-container); color: var(--on-error-container); border-color: var(--error); font-weight: 700;' : 'background: var(--primary-container); color: var(--on-primary-container); border-color: var(--primary);'">
                Active
              </span>
              <span v-else class="badge-pill">Select</span>
              <span class="expand-caret" :class="{ 'open': expandedRows[m.id] }" @click.stop="toggleExpand(m.id)">▼</span>
            </div>
          </div>

          <div class="expanded-content">
            <div class="expanded-inner">
              <div style="margin-bottom: 8px; font-family: var(--font-sans); color: var(--on-surface-variant); font-size: 11px;">
                {{ m.desc }}
              </div>
              <div class="stat-grid-2">
                <div class="stat-box">
                  <div class="stat-lbl">Est. Current</div>
                  <div class="stat-num">{{ m.estCurrent }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Est. Power</div>
                  <div class="stat-num">{{ m.estPower }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Node Target</div>
                  <div class="stat-num">{{ m.nodeVal }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Thermal Guard</div>
                  <div class="stat-num">{{ m.guardNote }}</div>
                </div>
              </div>
            </div>
          </div>
        </div>

      </div>

      <!-- Footer Info -->
      <div class="thermal-guard-footer">
        <Icons name="shield" :size="13" />
        <span>Thermal Guard Active &bull; Auto-protection &ge;45&deg;C</span>
      </div>

    </div>

    <!-- Violent Charging Risk Disclaimer Modal -->
    <transition name="modal-fade">
      <div v-if="showViolentModal" class="modal-backdrop" @click.self="showViolentModal = false">
        <div class="modal-card" style="padding: 20px;">
          <div style="display: flex; align-items: center; gap: 12px; margin-bottom: 12px;">
            <div class="icon-badge" style="background: var(--error-container); color: var(--on-error-container);">
              <Icons name="zap" :size="20" />
            </div>
            <div>
              <div style="font-size: 15px; font-weight: 700; color: var(--on-surface);">Violent Charge Warning</div>
              <div style="font-size: 11px; color: var(--on-surface-variant);">Hardware stress &amp; risk acknowledgement</div>
            </div>
          </div>

          <div style="font-size: 12px; color: var(--on-surface-variant); line-height: 1.5; margin-bottom: 16px;">
            Violent Charge forces maximum 33W fast charge protocol by disabling OEM smart charging limits and signaling extreme thermal profile to the kernel.<br/><br/>
            <strong>Important Risk Notice:</strong><br/>
            • Higher heat output may increase battery temperature under heavy load.<br/>
            • Prolonged exposure to high temperature can accelerate long-term battery capacity degradation.<br/>
            • Ensure you are using an official 33W factory charger and quality cable.
          </div>

          <div style="display: flex; gap: 8px; justify-content: flex-end;">
            <button class="btn-md3 btn-md3-secondary" @click="showViolentModal = false">
              Cancel
            </button>
            <button class="btn-md3 btn-md3-danger" @click="confirmViolentMode">
              I Understand &amp; Accept Risks
            </button>
          </div>
        </div>
      </div>
    </transition>

  </div>
</template>

<script setup>
import { ref, computed, inject, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const expandedRows = ref({})
const showViolentModal = ref(false)

const modes = [
  {
    id: 0,
    name: 'OEM Stock',
    desc: 'Full control managed by HyperOS / stock ROM driver. HyperCore does not interfere.',
    icon: 'device',
    badgeClass: 'tertiary',
    estCurrent: 'OEM Dynamic',
    estPower: 'Dynamic',
    nodeVal: 'Unrestricted',
    guardNote: 'ROM Driver'
  },
  {
    id: 1,
    name: 'Fast Charge',
    desc: 'Unrestricted maximum charging speed. Recommended when battery is low and fast top-up is needed.',
    icon: 'zap',
    badgeClass: '',
    estCurrent: '~3,300 – 3,500 mA',
    estPower: '~13.5 W',
    nodeVal: 'limit = 0',
    guardNote: 'Auto <45°C'
  },
  {
    id: 2,
    name: 'Balanced',
    desc: 'Stable ~2A charging rate with minimal heat. Best overall option for daily use.',
    icon: 'shield',
    badgeClass: 'secondary',
    estCurrent: '~1,900 – 2,000 mA',
    estPower: '~7.5 W',
    nodeVal: 'limit = 5',
    guardNote: 'Auto <45°C'
  },
  {
    id: 3,
    name: 'Safe Mode',
    desc: 'Cool ~1A trickle charging to protect battery health. Recommended for overnight charging.',
    icon: 'thermo',
    badgeClass: 'tertiary',
    estCurrent: '~1,000 – 1,050 mA',
    estPower: '~4.0 W',
    nodeVal: 'limit = 10',
    guardNote: 'Always Safe'
  },
  {
    id: 4,
    name: 'Bypass Mode',
    desc: 'Disconnects battery cell charging while supplying power directly to motherboard. Prevents heat while gaming.',
    icon: 'memory',
    badgeClass: '',
    estCurrent: '0 mA',
    estPower: '0 W',
    nodeVal: 'input_suspend = 1',
    guardNote: 'Bypass Cutoff'
  },
  {
    id: 5,
    name: 'Violent Charge',
    desc: 'Unlocks peak 33W fast charge protocol by signaling extreme thermal profile and disabling smart charge limits.',
    icon: 'rocket',
    badgeClass: '',
    estCurrent: '~5,000 – 6,600 mA',
    estPower: '~20.0 – 33.0 W',
    nodeVal: 'limit = 0 · sconfig = 0',
    guardNote: 'Extreme Heat'
  }
]

const activeModeName = computed(() => {
  if (!store.chargerSupported) return 'Unsupported'
  const m = modes.find(x => x.id === store.chargeMode)
  return m ? m.name : 'OEM Stock'
})

const activePowerSummary = computed(() => {
  if (!store.chargerSupported) return 'Hardware nodes unavailable'
  if (store.chargeModeOverride) return 'Thermal override active (Safe mode)'
  if (store.chargeMode === 4) return 'Bypass mode (Motherboard direct power)'
  if (store.chargeMode === 5) return 'Violent charge active (Peak 33W)'
  if (store.batStatus === 'Charging') return `${activeModeName.value} active &bull; Charging`
  return `${activeModeName.value} active &bull; Discharging`
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

function toggleExpand(id) {
  expandedRows.value[id] = !expandedRows.value[id]
}

function onSelectMode(id) {
  if (!store.chargerSupported) {
    if (toast) toast('Your device does not support this function properly')
    return
  }
  if (id === 5) {
    showViolentModal.value = true
  } else {
    applyMode(id)
  }
}

async function confirmViolentMode() {
  showViolentModal.value = false
  await applyMode(5)
}

async function applyMode(id) {
  if (!store.isRunning) {
    if (toast) toast('Daemon inactive — start daemon service first')
    return
  }
  await store.setChargeMode(id)
  const m = modes.find(x => x.id === id)
  if (toast) toast(`Charge mode switched to ${m?.name ?? id}`)
}

onMounted(() => {
  /* Trigger an immediate poll on mount, then rely on the store's unified
   * chargerInterval (startCardPolling) which already runs every 2s with
   * an isRefreshing guard. Running a separate localPollTimer would double
   * the polling rate and spawn redundant shell forks. */
  store.pollCharger()
  store.startCardPolling()
})

onUnmounted(() => {
  store.stopCardPolling()
})
</script>

<style scoped>
.selected-mode {
  background: var(--surface-container-high) !important;
  border-left: 3px solid var(--primary) !important;
}

.violent-mode-row {
  border-left-color: var(--error) !important;
}

.disabled-mode-row {
  opacity: 0.45;
  cursor: not-allowed !important;
}

.thermal-guard-footer {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  margin: 16px 0 24px 0;
  padding: 8px 14px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 20px;
  color: var(--on-surface-variant);
  font-size: 11px;
  font-weight: 500;
  opacity: 0.85;
}
</style>
