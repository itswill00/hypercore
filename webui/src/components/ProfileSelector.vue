<template>
  <div class="md3-card" style="padding: 12px; margin-bottom: 10px;">
    <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px;">
      <span style="font-size: 13px; font-weight: 600; color: var(--on-surface);">Operating profile</span>
      <span class="badge-pill purple" style="font-size: 10px; padding: 2px 8px;">{{ displayLabel }}</span>
    </div>

    <div class="segment-container">
      <button
        v-for="p in profiles"
        :key="p.mode"
        class="segment-btn"
        :class="{ 'active': isActive(p.mode) }"
        @click="select(p.mode)"
      >
        <Icons :name="p.icon" :size="13" />
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

function select(mode) {
  store.setProfile(mode)
  if (toast) toast(`Profile: ${mode === 'AUTO' ? 'Auto' : mode}`)
}
</script>
