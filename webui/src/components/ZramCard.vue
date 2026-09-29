<template>
  <div>
    <div class="section-title">Memory</div>
    <div class="md3-list-group">
      <div
        class="md3-list-row expandable-row clickable"
        :class="{ 'is-expanded': expanded }"
        @click="toggleExpand"
      >
        <div class="row-header">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="memory" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">ZRAM Swap Size</div>
              <div class="row-sub">{{ liveLine }}</div>
            </div>
          </div>
          <div class="row-val" style="margin-left: 8px;">
            <span class="badge-pill">{{ pendingLabel }}</span>
            <span class="expand-caret" :class="{ 'open': expanded }">▼</span>
          </div>
        </div>

        <div class="expanded-content" @click.stop>
          <div class="expanded-inner">
            <div class="zram-pill-row">
              <button
                v-for="pill in pills"
                :key="pill.key"
                class="zram-pill"
                :class="{ 'is-selected': isSelected(pill) }"
                :disabled="store.loading || cooldownLeft > 0"
                @click="apply(pill)"
              >
                <span>{{ pill.name }}</span>
                <span class="zram-pill-sub">{{ pill.sub }}</span>
              </button>
            </div>

            <div class="zram-note" style="margin-top: 8px;">{{ cooldownLeft > 0 ? `Saved. Wait ${cooldownLeft}s before changing again.` : 'Shrinking caps worst-case compression work and heat. It does not add free RAM instantly.' }}</div>

            <div class="zram-reboot-row" v-if="needsReboot">
              <div class="row-meta">
                <div class="row-title" style="font-size: 11px;">Reboot needed to apply</div>
                <div class="zram-note">Resizing moves compressed pages back to RAM, so it only runs once at boot for safety.</div>
              </div>
              <button class="zram-reboot-btn" @click="showRebootConfirm = true">Reboot</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <Teleport to="body">
      <Transition name="modal-fade">
        <div v-if="showRebootConfirm" class="modal-backdrop" @click.self="showRebootConfirm = false">
          <div class="modal-card" role="dialog" aria-modal="true" style="padding: 20px;">
            <div style="font-size: 15px; font-weight: 700; color: var(--on-surface); margin-bottom: 6px;">Reboot to apply?</div>
            <div class="zram-note" style="font-size: 12px; margin-bottom: 16px;">
              ZRAM pool changes from {{ livePoolLabel }} to {{ pendingPoolLabel }} on next boot. Save your work first.
            </div>
            <div class="zram-action-row">
              <button class="zram-btn-cancel" @click="showRebootConfirm = false">Cancel</button>
              <button class="zram-btn-danger" @click="confirmReboot">Reboot now</button>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>
  </div>
</template>

<script setup>
import { computed, inject, onMounted, onUnmounted, ref } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const COOLDOWN_S = 15
const COOLDOWN_KEY = 'hypercore_zram_cooldown'
const EXPAND_KEY = 'hypercore_zram_expanded'

const pills = [
  { key: 'stock', name: 'Stock', sub: 'ROM default' },
  { key: 4096, name: '4 GB', sub: 'Daily' },
  { key: 2048, name: '2 GB', sub: 'Gaming' },
  { key: 0, name: 'Off', sub: 'No swap' },
]

const cooldownLeft = ref(0)
const showRebootConfirm = ref(false)
const expanded = ref(true)
let cooldownTimer = null

function poolLabel(mb) {
  if (mb === null || mb === undefined) return 'Stock'
  if (mb === 0) return 'Off'
  return `${mb / 1024} GB`
}

const liveLine = computed(() => {
  const parts = []
  if (store.zramLiveMb > 0) {
    parts.push(`Live pool ${store.zramLiveMb >= 1024 ? (store.zramLiveMb / 1024).toFixed(1) + ' GB' : store.zramLiveMb + ' MB'}`)
    if (store.zramComp) parts.push(store.zramComp)
  } else {
    parts.push('Swap pool off')
  }
  if (store.zramUsage && store.zramUsage !== '—') parts.push(`used ${store.zramUsage.split(' ')[0]} GB`)
  return parts.join(' · ') || 'Compressed swap pool'
})

const livePoolLabel = computed(() => {
  if (store.zramLiveMb > 0) return `${(store.zramLiveMb / 1024).toFixed(1)} GB live`
  return 'Off'
})

const pendingPoolLabel = computed(() => poolLabel(store.zramConfiguredMb))

const pendingLabel = computed(() => {
  const v = store.zramConfiguredMb
  if (v === null || v === undefined) return 'Stock'
  if (v === 0) return 'Off on reboot'
  return `${v / 1024} GB on reboot`
})

const needsReboot = computed(() => {
  const v = store.zramConfiguredMb
  if (v === null || v === undefined) return false
  if (v === 0) return store.zramLiveMb !== 0
  return store.zramLiveMb !== v
})

function isSelected(pill) {
  if (pill.key === 'stock') return store.zramConfiguredMb === null || store.zramConfiguredMb === undefined
  return store.zramConfiguredMb === pill.key
}

function startCooldown() {
  try {
    localStorage.setItem(COOLDOWN_KEY, String(Date.now() + COOLDOWN_S * 1000))
  } catch {}
  tickCooldown()
}

function toggleExpand() {
  expanded.value = !expanded.value
  try {
    localStorage.setItem(EXPAND_KEY, expanded.value ? '1' : '0')
  } catch {}
}

function tickCooldown() {
  let until = 0
  try {
    until = parseInt(localStorage.getItem(COOLDOWN_KEY) || '0')
  } catch {}
  const left = Math.max(0, Math.ceil((until - Date.now()) / 1000))
  cooldownLeft.value = left
  if (cooldownTimer) {
    clearInterval(cooldownTimer)
    cooldownTimer = null
  }
  if (left > 0) {
    cooldownTimer = setInterval(() => {
      tickCooldown()
    }, 1000)
  }
}

async function apply(pill) {
  if (store.loading) return
  if (cooldownLeft.value > 0) {
    if (toast) toast(`Slow down — wait ${cooldownLeft.value}s`)
    return
  }
  const msg = await store.setZramSize(pill.key)
  if (msg && msg !== 'invalid') {
    startCooldown()
    if (!expanded.value) toggleExpand()
  }
  if (toast && msg) toast(msg)
}

async function confirmReboot() {
  showRebootConfirm.value = false
  const msg = await store.rebootDevice()
  if (toast && msg) toast(msg)
}

onMounted(() => {
  try {
    const saved = localStorage.getItem(EXPAND_KEY)
    if (saved !== null) expanded.value = saved === '1'
  } catch {}
  tickCooldown()
})

onUnmounted(() => {
  if (cooldownTimer) {
    clearInterval(cooldownTimer)
    cooldownTimer = null
  }
})
</script>

<style scoped>
.zram-pill-row {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 6px;
  width: 100%;
}

.zram-pill {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 7px 4px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 8px;
  color: var(--on-surface);
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease, color 0.15s ease;
  user-select: none;
  -webkit-tap-highlight-color: transparent;
  font-family: inherit;
}

.zram-pill span {
  font-size: 11px;
  font-weight: 600;
}

.zram-pill .zram-pill-sub {
  font-size: 9.5px;
  font-weight: 400;
  color: var(--on-surface-variant);
  font-variant-numeric: tabular-nums;
  margin-top: 2px;
}

.zram-pill:active {
  transform: scale(0.92);
  transition: transform 0.08s ease;
}

.zram-pill:disabled {
  opacity: 0.5;
}

.zram-pill.is-selected {
  background: var(--primary-container);
  border-color: var(--primary);
  color: var(--on-primary-container);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.25);
}

.zram-pill.is-selected .zram-pill-sub {
  color: var(--on-primary-container);
  opacity: 0.85;
}

.zram-note {
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.4;
  white-space: normal;
}

.zram-reboot-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-top: 10px;
  padding-top: 10px;
  border-top: 1px dashed var(--outline-variant);
}

.zram-reboot-btn {
  padding: 7px 16px;
  border-radius: 10px;
  border: none;
  background: var(--primary);
  color: var(--on-primary);
  font-size: 12px;
  font-weight: 700;
  font-family: inherit;
  cursor: pointer;
  -webkit-tap-highlight-color: transparent;
}

.zram-reboot-btn:active {
  transform: scale(0.95);
  filter: brightness(0.92);
}

.zram-action-row {
  display: flex;
  gap: 10px;
}

.zram-btn-cancel {
  flex: 1;
  padding: 11px;
  border-radius: 12px;
  border: 1px solid var(--outline-variant, #383a42);
  background: transparent;
  color: var(--on-surface-variant, #b0b4c0);
  font-size: 13px;
  font-weight: 600;
  font-family: inherit;
  cursor: pointer;
}

.zram-btn-cancel:active {
  transform: scale(0.96);
}

.zram-btn-danger {
  flex: 2;
  padding: 11px 16px;
  border-radius: 12px;
  border: none;
  background: var(--error, #ba1a1a);
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  font-family: inherit;
  cursor: pointer;
}

.zram-btn-danger:active {
  transform: scale(0.96);
  filter: brightness(0.9);
}
</style>
