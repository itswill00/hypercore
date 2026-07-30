<template>
  <nav class="md3-nav fixed bottom-0 left-0 right-0 w-full bg-surface-container shadow-lg z-50 backdrop-blur-md">
    <div class="max-w-md mx-auto h-16 flex items-center justify-around px-2">
      <router-link
        v-for="item in navItems"
        :key="item.name"
        :to="item.path"
        class="nav-btn flex flex-col items-center justify-center flex-1 no-underline transition-all duration-200"
        :class="{ 'active': isActive(item) }"
      >
        <div
          class="icon-wrapper h-8 flex items-center justify-center rounded-full transition-all duration-200"
          :class="isActive(item) ? 'bg-secondary-container text-on-secondary-container px-5' : 'text-on-surface-variant px-2'"
        >
          <Icons :name="item.icon" :size="20" />
        </div>
        <span
          class="nav-label text-xs mt-1 font-medium transition-colors"
          :class="isActive(item) ? 'text-on-surface font-semibold' : 'text-on-surface-variant'"
        >
          {{ item.label }}
        </span>
      </router-link>
    </div>
  </nav>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import Icons from '@/components/icons/Icons.vue'

const route = useRoute()

const navItems = [
  { name: 'Dashboard', path: '/', label: 'Dashboard', icon: 'home' },
  { name: 'Games', path: '/games', label: 'Games', icon: 'games' },
  { name: 'Logs', path: '/logs', label: 'Logs', icon: 'logs' }
]

const isActive = (item) => {
  if (item.path === '/') return route.path === '/'
  return route.path.startsWith(item.path)
}
</script>

<style scoped>
.md3-nav {
  border-top: 1px solid var(--surface-container-high);
}

.nav-btn {
  padding: 6px 0;
  user-select: none;
}

.icon-wrapper {
  transition: background-color 0.25s cubic-bezier(0.2, 0, 0, 1), padding 0.25s cubic-bezier(0.2, 0, 0, 1);
}

.nav-label {
  font-size: 11px;
  line-height: 1.2;
}
</style>
