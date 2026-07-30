<template>
  <div class="page-home pb-safe-nav overflow-y-auto h-full scrollbar-hidden">
    <!-- Header -->
    <div class="sticky top-0 z-10 bg-surface/90 backdrop-blur-md px-4 py-3 border-b border-surface-container-high mb-4">
      <div class="flex items-center justify-between">
        <div>
          <h1 class="text-lg font-bold text-on-surface">Dashboard</h1>
          <p class="text-xs text-on-surface-variant">Kernel tuner & gaming engine stats</p>
        </div>
        <button class="md3-btn md3-btn-secondary p-2 rounded-full" @click="store.refresh()">
          <Icons name="refresh" :size="18" />
        </button>
      </div>
    </div>

    <div class="px-4 max-w-md mx-auto">
      <ProfileSelector />
      <StatusCard />
      <ActionButtons />
    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import ProfileSelector from '@/components/ProfileSelector.vue'
import StatusCard from '@/components/StatusCard.vue'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
</script>
