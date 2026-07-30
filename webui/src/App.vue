<template>
  <div class="h-full w-full flex flex-col relative overflow-hidden bg-surface text-on-surface">
    <!-- Main Scroll View -->
    <main class="flex-1 overflow-hidden relative">
      <router-view v-slot="{ Component }">
        <transition name="page-fade" mode="out-in">
          <component :is="Component" />
        </transition>
      </router-view>
    </main>

    <!-- Bottom Navigation Bar -->
    <Navigation />

    <!-- Toast Notification -->
    <transition name="toast-slide">
      <div v-if="toastMsg" class="toast-pill fixed bottom-20 left-1/2 -translate-x-1/2 bg-surface-container-highest border border-surface-bright text-on-surface text-xs font-medium px-4 py-2 rounded-full shadow-lg z-50 pointer-events-none flex items-center gap-2">
        <Icons name="check" :size="14" class="text-primary" />
        <span>{{ toastMsg }}</span>
      </div>
    </transition>
  </div>
</template>

<script setup>
import { ref, provide, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Navigation from '@/components/Navigation.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toastMsg = ref('')
let toastTimer = null
let refreshInterval = null

function toast(msg) {
  toastMsg.value = msg
  if (toastTimer) clearTimeout(toastTimer)
  toastTimer = setTimeout(() => { toastMsg.value = '' }, 2500)
}

provide('toast', toast)

onMounted(() => {
  store.refresh()
  refreshInterval = setInterval(() => store.refresh(), 3000)
})

onUnmounted(() => {
  if (refreshInterval) clearInterval(refreshInterval)
})
</script>

<style scoped>
.toast-slide-enter-active,
.toast-slide-leave-active {
  transition: all 0.25s cubic-bezier(0.2, 0, 0, 1);
}

.toast-slide-enter-from {
  opacity: 0;
  transform: translate(-50%, 12px);
}

.toast-slide-leave-to {
  opacity: 0;
  transform: translate(-50%, -8px);
}
</style>
