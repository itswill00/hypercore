<template>
  <div class="md3-card">
    <div style="display: flex; align-items: center; justify-content: space-between;">
      <div>
        <div style="font-size: 14px; font-weight: 600; color: var(--on-surface);">Profile mode</div>
        <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 1px;">Current operating mode</div>
      </div>
      <span class="badge-pill purple">{{ displayLabel }}</span>
    </div>

    <div class="profile-grid">
      <button
        v-for="p in profiles"
        :key="p.mode"
        class="profile-btn"
        :class="{ 'active': isActive(p.mode) }"
        @click="select(p.mode)"
      >
        <Icons :name="p.icon" :size="16" style="margin-bottom: 4px;" />
        <span>{{ p.label }}</span>
      </button>
    </div>
  </div>
</template>

<script setup>
import { computed, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

const profiles = [
  { mode: 'AUTO', label: 'Auto', icon: 'star' },
  { mode: 'GAMING', label: 'Gaming', icon: 'rocket' },
  { mode: 'INTERACTIVE', label: 'Balanced', icon: 'cpu' },
  { mode: 'TOUCH', label: 'Touch', icon: 'zap' },
  { mode: 'SLEEP', label: 'Saver', icon: 'power' }
]

const normalized = computed(() => {
  let m = store.lockMode
  if (m === 'BALANCED') m = 'INTERACTIVE'
  if (m === 'SAVER') m = 'SLEEP'
  return m
})

const displayLabel = computed(() => {
  return normalized.value === 'AUTO' ? 'Auto' : `${normalized.value.charAt(0)}${normalized.value.slice(1).toLowerCase()} locked`
})

function isActive(mode) {
  return normalized.value === mode
}

async function select(mode) {
  await store.setProfile(mode)
  if (toast) toast(`Profile: ${mode === 'AUTO' ? 'Auto' : mode}`)
}
</script>
