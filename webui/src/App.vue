<template>
  <div class="shell">
    <header class="header">
      <div>
        <h1>Tanzanite HyperCore v3.4</h1>
        <p class="sub">Kernel tuner for MediaTek Helio G99 Ultra</p>
      </div>
    </header>

    <nav class="nav">
      <router-link to="/" class="nav-item" :class="{ active: $route.path === '/' }">Dashboard</router-link>
      <router-link to="/games" class="nav-item" :class="{ active: $route.path === '/games' }">Games</router-link>
      <router-link to="/logs" class="nav-item" :class="{ active: $route.path === '/logs' }">Logs</router-link>
    </nav>

    <main>
      <router-view v-slot="{ Component }">
        <transition name="fade" mode="out-in">
          <component :is="Component" />
        </transition>
      </router-view>
    </main>

    <footer class="footer">Tanzanite HyperCore v3.4 · @itswill00</footer>

    <!-- Toast -->
    <transition name="toast">
      <div v-if="toastMsg" class="toast">{{ toastMsg }}</div>
    </transition>
  </div>
</template>

<script setup>
import { ref, provide, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'

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
.shell {
  max-width: 520px;
  margin: 0 auto;
  padding: 16px;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.header h1 {
  font-size: 17px;
  font-weight: 600;
  letter-spacing: -0.01em;
  color: #fff;
  margin: 0;
}
.sub {
  margin-top: 2px;
  color: var(--text-muted);
  font-size: 11px;
  font-family: var(--mono);
}

.nav {
  display: flex;
  gap: 2px;
  margin: 14px 0 16px;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 3px;
}
.nav-item {
  flex: 1;
  text-align: center;
  padding: 7px 0;
  font-size: 12px;
  font-weight: 500;
  color: var(--text-muted);
  text-decoration: none;
  border-radius: 6px;
  transition: all 0.15s var(--ease, ease);
}
.nav-item:hover { color: var(--text); }
.nav-item.active {
  background: var(--bg);
  color: #fff;
  font-weight: 600;
}

main { flex: 1; }

.footer {
  text-align: center;
  font-size: 10px;
  color: var(--text-muted);
  font-family: var(--mono);
  margin-top: 16px;
  padding-top: 12px;
  border-top: 1px solid var(--border);
}

.toast {
  position: fixed;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--surface);
  border: 1px solid var(--border);
  color: var(--text-dim);
  font-size: 11px;
  font-family: var(--mono);
  padding: 8px 16px;
  border-radius: var(--radius);
  z-index: 9999;
  pointer-events: none;
}
.toast-enter-active { transition: all 0.2s ease; }
.toast-leave-active { transition: all 0.15s ease; }
.toast-enter-from { opacity: 0; transform: translateX(-50%) translateY(8px); }
.toast-leave-to { opacity: 0; transform: translateX(-50%) translateY(-4px); }
</style>
