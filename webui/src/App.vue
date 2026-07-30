<template>
  <div class="app-shell">
    <!-- Main Scroll View -->
    <main style="flex: 1; overflow: hidden; position: relative;">
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
      <div v-if="toastMsg" class="toast-pill">
        <Icons name="check" :size="14" style="color: var(--primary);" />
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
.toast-pill {
  position: fixed;
  bottom: 76px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--surface-container-highest);
  border: 1px solid var(--surface-bright);
  color: var(--on-surface);
  font-size: 12px;
  font-weight: 500;
  padding: 8px 16px;
  border-radius: 20px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
  z-index: 99;
  pointer-events: none;
  display: flex;
  align-items: center;
  gap: 8px;
}

.toast-slide-enter-active,
.toast-slide-leave-active {
  transition: all 0.2s cubic-bezier(0.2, 0, 0, 1);
}

.toast-slide-enter-from {
  opacity: 0;
  transform: translate(-50%, 10px);
}

.toast-slide-leave-to {
  opacity: 0;
  transform: translate(-50%, -6px);
}
</style>
