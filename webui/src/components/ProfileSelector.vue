<template>
  <div class="card">
    <div class="card-title">
      <span>Profile mode</span>
      <span class="badge">{{ displayLabel }}</span>
    </div>
    <div class="pill-grid">
      <button
        v-for="p in profiles"
        :key="p.mode"
        class="pill"
        :data-active="isActive(p.mode) ? p.style : undefined"
        @click="select(p.mode)"
      >{{ p.label }}</button>
    </div>
  </div>
</template>

<script setup>
import { computed, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
const toast = inject('toast')

const profiles = [
  { mode: 'AUTO', label: 'Auto', style: 'auto' },
  { mode: 'GAMING', label: 'Gaming', style: 'gaming' },
  { mode: 'INTERACTIVE', label: 'Balanced', style: 'balanced' },
  { mode: 'TOUCH', label: 'Touch', style: 'touch' },
  { mode: 'SLEEP', label: 'Saver', style: 'saver' }
]

const normalized = computed(() => {
  let m = store.lockMode
  if (m === 'BALANCED') m = 'INTERACTIVE'
  if (m === 'SAVER') m = 'SLEEP'
  return m
})

const displayLabel = computed(() => {
  return normalized.value === 'AUTO' ? 'Auto' : `${normalized.value.charAt(0)}${normalized.value.slice(1).toLowerCase()} (locked)`
})

function isActive(mode) { return normalized.value === mode }

async function select(mode) {
  await store.setProfile(mode)
  toast(`Profile: ${mode === 'AUTO' ? 'Auto' : mode}`)
}
</script>

<style scoped>
.pill-grid {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 6px;
  margin-top: 8px;
}
.pill {
  background: var(--bg);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 8px 4px;
  text-align: center;
  font-size: 11px;
  font-weight: 500;
  cursor: pointer;
  color: var(--text-muted);
  transition: all 0.15s ease;
  user-select: none;
  font-family: var(--sans);
}
.pill:hover { border-color: var(--border-hover); color: var(--text); }
.pill:active { transform: scale(0.96); }
.pill[data-active="auto"]     { background: rgba(59,130,246,0.12); border-color: var(--accent); color: var(--accent-soft); font-weight: 600; }
.pill[data-active="gaming"]   { background: rgba(239,68,68,0.12); border-color: var(--red); color: #f87171; font-weight: 600; }
.pill[data-active="balanced"] { background: rgba(34,197,94,0.12); border-color: var(--green); color: #4ade80; font-weight: 600; }
.pill[data-active="touch"]    { background: rgba(168,85,247,0.12); border-color: var(--purple); color: #c084fc; font-weight: 600; }
.pill[data-active="saver"]    { background: rgba(245,158,11,0.12); border-color: var(--amber); color: #fbbf24; font-weight: 600; }
</style>
