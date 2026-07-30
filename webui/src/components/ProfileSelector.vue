<template>
  <div class="md3-group p-4">
    <div class="flex items-center justify-between mb-3">
      <div>
        <h2 class="text-sm font-semibold text-on-surface">Profile Mode</h2>
        <p class="text-xs text-on-surface-variant mt-0.5">Select manual lock or dynamic auto governor</p>
      </div>
      <span class="px-2.5 py-1 text-xs font-semibold rounded-full bg-secondary-container text-on-secondary-container">
        {{ displayLabel }}
      </span>
    </div>

    <div class="grid grid-cols-5 gap-2 mt-2">
      <button
        v-for="p in profiles"
        :key="p.mode"
        class="profile-pill flex flex-col items-center justify-center py-2.5 px-1 rounded-xl text-xs font-medium transition-all duration-200 cursor-pointer border border-transparent"
        :class="isActive(p.mode) ? 'active-pill shadow-sm' : 'bg-surface-container-high text-on-surface-variant hover:bg-surface-bright'"
        @click="select(p.mode)"
      >
        <Icons :name="p.icon" :size="16" class="mb-1" />
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
  return normalized.value === 'AUTO' ? 'Auto Dynamic' : `${normalized.value.charAt(0)}${normalized.value.slice(1).toLowerCase()} Locked`
})

function isActive(mode) {
  return normalized.value === mode
}

async function select(mode) {
  await store.setProfile(mode)
  if (toast) toast(`Profile: ${mode === 'AUTO' ? 'Auto Dynamic' : mode}`)
}
</script>

<style scoped>
.active-pill {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-color: var(--primary);
  font-weight: 600;
}
</style>
