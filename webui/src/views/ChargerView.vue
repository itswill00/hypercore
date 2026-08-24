<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">Charger Control</div>
        <div class="page-header-sub">Charging speed, thermal guard &amp; bypass mode</div>
      </div>
      <span class="badge-pill" style="font-size: 11px; padding: 4px 10px;">
        {{ activeModeName }}
      </span>
    </div>

    <div class="content-area">

      <!-- Thermal Override Alert Banner (if thermal guard triggered) -->
      <div v-if="store.chargeModeOverride" class="md3-banner" style="background: var(--error-container); color: var(--on-error-container); border-color: var(--error); margin-bottom: 14px;">
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

      <!-- Real-Time Charging Telemetry Group -->
      <div class="section-title">Live Telemetry</div>
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="zap" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Charging Status</div>
              <div class="row-sub">Current flow &amp; input power</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill">{{ store.batStatus }}</span>
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
                <div class="stat-num">{{ store.batTemp }}°C</div>
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
            'selected-mode': store.chargeMode === m.id
          }"
          @click="selectMode(m.id)"
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
              <span v-if="store.chargeMode === m.id" class="badge-pill" style="background: var(--primary-container); color: var(--on-primary-container); border-color: var(--primary);">
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
                  <div class="stat-lbl">Target Arus</div>
                  <div class="stat-num">{{ m.estCurrent }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Target Daya</div>
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
      <div style="text-align: center; font-size: 10px; color: var(--on-surface-variant); opacity: 0.5; padding: 12px 0 24px 0; font-family: var(--font-mono);">
        Safety override automatically drops to Safe mode at &ge; 45&deg;C battery temp.
      </div>

    </div>
  </div>
</template>

<script setup>
import { ref, computed, inject, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const expandedRows = ref({})
let localPollTimer = null

const modes = [
  {
    id: 0,
    name: 'OEM Stock',
    desc: 'Kontrol penuh diserahkan ke driver bawaan HyperOS/ROM. Tidak ada intervensi dari HyperCore.',
    icon: 'device',
    badgeClass: 'tertiary',
    estCurrent: 'Dinamis OEM',
    estPower: 'Dinamis',
    nodeVal: 'Tidak diatur',
    guardNote: 'Driver ROM'
  },
  {
    id: 1,
    name: 'Fast Charge',
    desc: 'Pengisian maksimal tanpa limit arus. Cocok saat baterai kritis dan butuh pengisian cepat.',
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
    desc: 'Pengisian ~2A stabil dan dingin. Rekomendasi terbaik untuk penggunaan harian.',
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
    desc: 'Pengisian dingin ~1A untuk menjaga umur panjang baterai. Sangat disarankan saat pengisian malam/tidur.',
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
    desc: 'Sel baterai diputus dari pengisian. Seluruh daya dialirkan langsung ke komponen mesin HP untuk mencegah panas saat main game.',
    icon: 'memory',
    badgeClass: '',
    estCurrent: '0 mA',
    estPower: '0 W',
    nodeVal: 'input_suspend = 1',
    guardNote: 'Bypass Cutoff'
  }
]

const activeModeName = computed(() => {
  const m = modes.find(x => x.id === store.chargeMode)
  return m ? m.name : 'OEM Stock'
})

const displayCurrentMa = computed(() => {
  const ma = store.chargeCurrentMa
  if (!ma || ma === 0) return store.batStatus === 'Charging' ? '— mA' : '0 mA'
  return `${ma.toLocaleString()} mA`
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

async function selectMode(id) {
  if (!store.isRunning) {
    if (toast) toast('Daemon tidak aktif — jalankan daemon terlebih dahulu')
    return
  }
  await store.setChargeMode(id)
  const m = modes.find(x => x.id === id)
  if (toast) toast(`Charge mode switched to ${m?.name ?? id}`)
}

onMounted(() => {
  store.pollCharger()
  localPollTimer = setInterval(() => store.pollCharger(), 2000)
})

onUnmounted(() => {
  if (localPollTimer) {
    clearInterval(localPollTimer)
    localPollTimer = null
  }
})
</script>

<style scoped>
.selected-mode {
  background: var(--surface-container-high) !important;
  border-left: 3px solid var(--primary) !important;
}
</style>
