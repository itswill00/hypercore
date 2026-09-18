<template>
  <nav class="md3-navbar">
    <button
      v-for="item in navItems"
      :key="item.name"
      class="nav-item"
      :class="{ 'active': isActive(item) }"
      @click="$emit('navigate', item.path)"
    >
      <div class="nav-icon-wrapper">
        <Icons :name="item.icon" :size="20" />
      </div>
      <span class="nav-label">{{ item.label }}</span>
    </button>
  </nav>
</template>

<script setup>
import Icons from '@/components/icons/Icons.vue'
const props = defineProps({ activePath: { type: String, default: '/' } })
defineEmits(['navigate'])
const navItems = [
  { name: 'Dashboard', path: '/', label: 'Dashboard', icon: 'home' },
  { name: 'Charger', path: '/charger', label: 'Charger', icon: 'plug' },
  { name: 'Games', path: '/games', label: 'Games', icon: 'games' },
  { name: 'About', path: '/about', label: 'About', icon: 'about' },
  { name: 'Logs', path: '/logs', label: 'Logs', icon: 'logs' }
]
const isActive = (item) => {
  if (item.path === '/') return props.activePath === '/'
  return props.activePath.startsWith(item.path)
}
</script>
