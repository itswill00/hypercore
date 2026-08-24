<template>
  <div class="charger-view">

    <!-- Header -->
    <div class="charger-header">
      <div class="header-title">
        <Icons name="plug" :size="20" style="color: var(--primary);" />
        <span>Charger Control</span>
      </div>
      <div class="header-status" :class="statusClass">
        <span class="status-dot"></span>
        <span>{{ headerStatusText }}</span>
      </div>
    </div>

    <!-- Thermal Override Warning -->
    <transition name="fade-slide">
      <div v-if="store.chargeModeOverride" class="override-banner">
        <Icons name="thermal" :size="16" />
        <div>
          <strong>Thermal Safety Override Aktif</strong>
          <p>Daemon menurunkan mode sementara karena suhu baterai tinggi ({{ store.batTemp }}°C). Mode akan dikembalikan ke <em>{{ selectedModeName }}</em> setelah suhu turun di bawah 40°C.</p>
        </div>
      </div>
    </transition>

    <!-- Real-time Telemetry Bar -->
    <div class="telemetry-bar">
      <div class="tele-item">
        <span class="tele-label">Arus Masuk</span>
        <span class="tele-value" :class="currentClass">{{ displayCurrentMa }}</span>
      </div>
      <div class="tele-divider"></div>
      <div class="tele-item">
        <span class="tele-label">Tegangan</span>
        <span class="tele-value">{{ displayVoltage }}</span>
      </div>
      <div class="tele-divider"></div>
      <div class="tele-item">
        <span class="tele-label">Daya</span>
        <span class="tele-value" :class="currentClass">{{ displayWatt }}</span>
      </div>
      <div class="tele-divider"></div>
      <div class="tele-item">
        <span class="tele-label">Suhu Baterai</span>
        <span class="tele-value" :class="tempClass">{{ store.batTemp }}°C</span>
      </div>
    </div>

    <!-- Mode Cards -->
    <div class="mode-section-label">Pilih Mode Pengisian</div>

    <div class="mode-cards">
      <div
        v-for="m in modes"
        :key="m.id"
        class="mode-card"
        :class="{
          active: effectiveMode === m.id,
          overridden: store.chargeModeOverride && store.chargeMode === m.id && effectiveMode !== m.id
        }"
        @click="selectMode(m.id)"
      >
        <div class="card-top">
          <div class="card-icon-wrapper" :style="{ background: m.iconBg }">
            <Icons :name="m.icon" :size="18" :style="{ color: m.iconColor }" />
          </div>
          <div v-if="effectiveMode === m.id" class="card-active-badge">
            <Icons name="check" :size="10" /> Aktif
          </div>
          <div v-else-if="store.chargeMode === m.id && store.chargeModeOverride" class="card-override-badge">
            Override
          </div>
        </div>

        <div class="card-name">{{ m.name }}</div>
        <div class="card-desc">{{ m.desc }}</div>

        <div class="card-stats">
          <div class="stat-row">
            <span class="stat-label">Est. Arus</span>
            <span class="stat-val" :style="{ color: m.iconColor }">{{ m.estCurrent }}</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Est. Daya</span>
            <span class="stat-val" :style="{ color: m.iconColor }">{{ m.estPower }}</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Node Value</span>
            <span class="stat-val mono">{{ m.nodeVal }}</span>
          </div>
        </div>

        <div class="card-bar">
          <div class="card-bar-fill" :style="{ width: m.barPct + '%', background: m.iconColor }"></div>
        </div>
      </div>
    </div>

    <!-- Footer -->
    <div class="charger-footer">
      <Icons name="shield" :size="13" style="color: var(--primary); opacity: 0.7;" />
      <span>Safety override otomatis aktif pada suhu &ge; 45&deg;C dan akan pulih saat &lt; 40&deg;C.</span>
    </div>

  </div>
</template>

<script setup>
import { computed, inject, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

let localPollTimer = null

const modes = [
  {
    id: 0,
    name: 'OEM Stock',
    desc: 'Kontrol penuh ke driver bawaan HyperOS/ROM. Tidak ada intervensi dari HyperCore.',
    icon: 'device',
    iconColor: 'var(--on-surface-variant)',
    iconBg: 'rgba(255,255,255,0.06)',
    estCurrent: 'Dinamis OEM',
    estPower: 'Dinamis',
    nodeVal: 'tidak diatur',
    barPct: 45
  },
  {
    id: 1,
    name: 'Fast Charge',
    desc: 'Pengisian maksimal tanpa batas. Gunakan saat baterai kritis dan tidak perlu bermain.',
    icon: 'gpu',
    iconColor: '#FF6B6B',
    iconBg: 'rgba(255,107,107,0.12)',
    estCurrent: '~3,300 – 3,500 mA',
    estPower: '~13.5 W',
    nodeVal: 'limit = 0',
    barPct: 100
  },
  {
    id: 2,
    name: 'Balanced',
    desc: 'Pengisian ~2A stabil, suhu terjaga. Pilihan terbaik untuk penggunaan harian.',
    icon: 'shield',
    iconColor: '#4ECDC4',
    iconBg: 'rgba(78,205,196,0.12)',
    estCurrent: '~1,900 – 2,000 mA',
    estPower: '~7.5 W',
    nodeVal: 'limit = 5',
    barPct: 58
  },
  {
    id: 3,
    name: 'Safe',
    desc: 'Pengisian lambat ~1A untuk kesehatan baterai jangka panjang. Ideal saat tidur.',
    icon: 'thermal',
    iconColor: '#45B7D1',
    iconBg: 'rgba(69,183,209,0.12)',
    estCurrent: '~1,000 – 1,050 mA',
    estPower: '~4.0 W',
    nodeVal: 'limit = 10',
    barPct: 30
  },
  {
    id: 4,
    name: 'Bypass',
    desc: 'Sel baterai diputus dari input daya. HP berjalan langsung dari adaptor. Ideal gaming panas.',
    icon: 'power',
    iconColor: '#A78BFA',
    iconBg: 'rgba(167,139,250,0.12)',
    estCurrent: '0 mA',
    estPower: '0 W',
    nodeVal: 'input_suspend = 1',
    barPct: 0
  }
]

const effectiveMode = computed(() => store.chargeMode)

const selectedModeName = computed(() => {
  const m = modes.find(x => x.id === store.chargeMode)
  return m ? m.name : 'Balanced'
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

const headerStatusText = computed(() => {
  if (!store.isRunning) return 'Daemon tidak aktif'
  if (store.chargeModeOverride) return 'Safety Override Aktif'
  const m = modes.find(x => x.id === effectiveMode.value)
  return m ? m.name : 'OEM Stock'
})

const statusClass = computed(() => {
  if (!store.isRunning) return 'status-off'
  if (store.chargeModeOverride) return 'status-warn'
  if (effectiveMode.value === 1) return 'status-fast'
  if (effectiveMode.value === 4) return 'status-bypass'
  return 'status-ok'
})

const currentClass = computed(() => {
  const ma = store.chargeCurrentMa
  if (ma > 3000) return 'val-hot'
  if (ma > 1500) return 'val-warm'
  return 'val-cool'
})

const tempClass = computed(() => {
  if (store.batTemp >= 45) return 'val-hot'
  if (store.batTemp >= 40) return 'val-warm'
  return 'val-cool'
})

async function selectMode(id) {
  if (!store.isRunning) {
    toast('Daemon tidak aktif — hidupkan daemon terlebih dahulu')
    return
  }
  await store.setChargeMode(id)
  const m = modes.find(x => x.id === id)
  toast(`Mode diubah ke: ${m?.name ?? id}`)
}

onMounted(() => {
  store.pollCharger()
  localPollTimer = setInterval(() => store.pollCharger(), 2000)
})

onUnmounted(() => {
  if (localPollTimer) { clearInterval(localPollTimer); localPollTimer = null }
})
</script>

<style scoped>
.charger-view {
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 16px 14px 80px;
  min-height: 100vh;
  background: var(--background);
  overflow-y: auto;
}

/* ── Header ── */
.charger-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 0;
}
.header-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 17px;
  font-weight: 700;
  color: var(--on-surface);
  letter-spacing: -0.3px;
}
.header-status {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  font-weight: 600;
  padding: 4px 10px;
  border-radius: 20px;
  transition: all 0.3s ease;
}
.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: currentColor;
  animation: pulse-dot 2s infinite;
}
@keyframes pulse-dot {
  0%, 100% { opacity: 1; }
  50%       { opacity: 0.4; }
}
.status-ok     { background: rgba(78,205,196,0.15);  color: #4ECDC4; }
.status-fast   { background: rgba(255,107,107,0.15); color: #FF6B6B; }
.status-warn   { background: rgba(251,191,36,0.15);  color: #FBBF24; }
.status-bypass { background: rgba(167,139,250,0.15); color: #A78BFA; }
.status-off    { background: rgba(255,255,255,0.06); color: var(--on-surface-variant); }

/* ── Override Banner ── */
.override-banner {
  display: flex;
  align-items: flex-start;
  gap: 10px;
  background: rgba(251,191,36,0.08);
  border: 1px solid rgba(251,191,36,0.3);
  border-radius: 12px;
  padding: 12px;
  color: #FBBF24;
  font-size: 12px;
  line-height: 1.5;
}
.override-banner strong { display: block; font-weight: 700; margin-bottom: 3px; }
.override-banner p  { margin: 0; opacity: 0.85; }
.override-banner em { font-style: normal; font-weight: 600; }

.fade-slide-enter-active,
.fade-slide-leave-active { transition: all 0.3s ease; }
.fade-slide-enter-from,
.fade-slide-leave-to    { opacity: 0; transform: translateY(-6px); }

/* ── Telemetry Bar ── */
.telemetry-bar {
  display: flex;
  align-items: stretch;
  background: var(--surface-container);
  border: 1px solid var(--surface-bright);
  border-radius: 14px;
  padding: 12px 0;
}
.tele-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 0 6px;
}
.tele-label {
  font-size: 10px;
  font-weight: 500;
  color: var(--on-surface-variant);
  letter-spacing: 0.2px;
  white-space: nowrap;
}
.tele-value {
  font-size: 13px;
  font-weight: 700;
  color: var(--on-surface);
  transition: color 0.4s ease;
}
.tele-divider {
  width: 1px;
  background: var(--surface-bright);
  margin: 2px 0;
}

.val-hot  { color: #FF6B6B !important; }
.val-warm { color: #FBBF24 !important; }
.val-cool { color: #4ECDC4 !important; }

/* ── Section Label ── */
.mode-section-label {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
  letter-spacing: 0.6px;
  text-transform: uppercase;
  padding: 0 2px;
}

/* ── Mode Cards ── */
.mode-cards {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.mode-card {
  background: var(--surface-container);
  border: 1.5px solid var(--surface-bright);
  border-radius: 16px;
  padding: 14px 14px 10px;
  cursor: pointer;
  transition: all 0.22s cubic-bezier(0.2, 0, 0, 1);
  position: relative;
  -webkit-tap-highlight-color: transparent;
}
.mode-card:active { transform: scale(0.985); }

.mode-card.active {
  border-color: var(--primary);
  background: var(--surface-container-high);
  box-shadow: 0 0 0 1px var(--primary), 0 4px 20px rgba(0,0,0,0.25);
}
.mode-card.overridden {
  border-color: rgba(251,191,36,0.45);
}

/* Top row: icon + badge */
.card-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 10px;
}
.card-icon-wrapper {
  width: 36px;
  height: 36px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: transform 0.2s ease;
}
.mode-card.active .card-icon-wrapper { transform: scale(1.08); }

.card-active-badge {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 10px;
  font-weight: 700;
  color: var(--primary);
  background: color-mix(in srgb, var(--primary) 15%, transparent);
  padding: 3px 9px;
  border-radius: 20px;
}
.card-override-badge {
  font-size: 10px;
  font-weight: 700;
  color: #FBBF24;
  background: rgba(251,191,36,0.12);
  padding: 3px 9px;
  border-radius: 20px;
}

.card-name {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface);
  margin-bottom: 4px;
}
.card-desc {
  font-size: 11px;
  line-height: 1.5;
  color: var(--on-surface-variant);
  margin-bottom: 10px;
}

/* Stats */
.card-stats {
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin-bottom: 10px;
}
.stat-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.stat-label {
  font-size: 10.5px;
  color: var(--on-surface-variant);
  font-weight: 500;
}
.stat-val {
  font-size: 11px;
  font-weight: 700;
  color: var(--on-surface);
}
.stat-val.mono {
  font-family: 'Courier New', monospace;
  font-size: 10px;
  color: var(--on-surface-variant);
}

/* Bar */
.card-bar {
  height: 3px;
  background: var(--surface-bright);
  border-radius: 2px;
  overflow: hidden;
}
.card-bar-fill {
  height: 100%;
  border-radius: 2px;
  transition: width 0.6s cubic-bezier(0.2, 0, 0, 1);
  opacity: 0.55;
}
.mode-card.active .card-bar-fill { opacity: 1; }

/* Footer */
.charger-footer {
  display: flex;
  align-items: flex-start;
  gap: 7px;
  font-size: 10.5px;
  color: var(--on-surface-variant);
  line-height: 1.5;
  opacity: 0.65;
  padding: 0 2px 4px;
}
</style>
