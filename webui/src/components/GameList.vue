<template>
  <div class="mb-6">
    <div class="flex items-center justify-between mb-3 px-1">
      <div>
        <h2 class="text-sm font-semibold text-on-surface">Configured Applications</h2>
        <p class="text-xs text-on-surface-variant mt-0.5">Apps that trigger gaming boost or custom profiles</p>
      </div>
      <span class="px-2.5 py-1 text-xs font-semibold rounded-full bg-secondary-container text-on-secondary-container">
        {{ store.games.length }} {{ store.games.length === 1 ? 'App' : 'Apps' }}
      </span>
    </div>

    <!-- Game List Group -->
    <div class="md3-group">
      <div v-if="store.games.length === 0" class="p-6 text-center text-xs text-on-surface-variant">
        No custom apps or games configured. Add packages below to trigger profiles automatically.
      </div>
      <div v-for="entry in store.games" :key="entry" class="md3-list-item">
        <div class="flex items-center gap-3 overflow-hidden min-w-0 flex-1">
          <img :src="iconFor(entry)" class="w-8 h-8 rounded-lg bg-surface-container-high object-cover shrink-0" @error="e => e.target.style.visibility = 'hidden'" loading="lazy">
          <div class="min-w-0 flex-1">
            <div class="item-title truncate font-mono text-xs">{{ entry.split(':')[0] }}</div>
            <div class="item-sub flex items-center gap-1.5 mt-0.5">
              <span class="px-1.5 py-0.5 text-[10px] font-semibold rounded bg-primary-container text-on-primary-container">
                {{ getMode(entry) }}
              </span>
            </div>
          </div>
        </div>
        <div class="flex items-center gap-2 shrink-0 ml-2">
          <button class="md3-btn md3-btn-primary text-xs py-1.5 px-3" @click="launch(entry)">
            <Icons name="rocket" :size="14" />
            <span>Launch</span>
          </button>
          <button class="md3-btn md3-btn-danger text-xs py-1.5 px-2.5" @click="remove(entry)">
            <Icons name="trash" :size="14" />
          </button>
        </div>
      </div>
    </div>

    <!-- Add Application Box -->
    <div class="md3-group p-4 bg-surface-container">
      <h3 class="text-xs font-semibold text-on-surface mb-2">Add New Application</h3>
      <div class="flex gap-2">
        <input v-model="newPkg" type="text" class="md3-input flex-1" placeholder="com.package.name or com.pkg:TOUCH" @keydown.enter="add">
        <button class="md3-btn md3-btn-secondary" @click="add">
          <Icons name="plus" :size="16" />
          <span>Add</span>
        </button>
        <button class="md3-btn md3-btn-primary" @click="showPicker = true">
          <Icons name="device" :size="16" />
          <span>Select</span>
        </button>
      </div>
    </div>
  </div>

  <!-- App Picker Modal -->
  <AppPicker v-if="showPicker" @close="showPicker = false" @pick="onPick" />
</template>

<script setup>
import { ref, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { getIconUrl } from '@/helpers/shell'
import AppPicker from '@/components/AppPicker.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')
const newPkg = ref('')
const showPicker = ref(false)

function iconFor(entry) {
  return getIconUrl(entry.split(':')[0].trim())
}

function getMode(entry) {
  const parts = entry.split(':')
  return parts.length > 1 ? parts[1].trim().toUpperCase() : 'GAMING'
}

async function add() {
  if (!newPkg.value.trim()) return
  const msg = await store.addGame(newPkg.value.trim())
  if (msg && toast) toast(msg)
  newPkg.value = ''
}

async function remove(entry) {
  const msg = await store.removeGame(entry)
  if (msg && toast) toast(msg)
}

async function launch(entry) {
  const msg = await store.launchGame(entry)
  if (msg && toast) toast(msg)
}

async function onPick(pkg) {
  showPicker.value = false
  const msg = await store.addGame(pkg)
  if (msg && toast) toast(msg)
}
</script>
