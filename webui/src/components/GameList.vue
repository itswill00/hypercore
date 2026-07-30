<template>
  <div>
    <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; padding: 0 4px;">
      <div>
        <div style="font-size: 14px; font-weight: 600; color: var(--on-surface);">My games</div>
        <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 1px;">Apps with auto boost & profile triggers</div>
      </div>
      <span class="badge-pill purple">{{ store.games.length }} {{ store.games.length === 1 ? 'app' : 'apps' }}</span>
    </div>

    <!-- Game List Group -->
    <div class="md3-list-group">
      <div v-if="store.games.length === 0" style="padding: 24px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
        No custom apps or games configured. Add packages below to trigger profiles automatically.
      </div>
      <div v-for="entry in store.games" :key="entry" class="md3-list-row">
        <div class="row-left">
          <img :src="iconFor(entry)" style="width: 32px; height: 32px; border-radius: 8px; background: var(--surface-container-high); object-fit: cover; flex-shrink: 0;" @error="e => e.target.style.visibility = 'hidden'" loading="lazy">
          <div class="row-meta">
            <div class="row-title" style="font-family: var(--font-mono); font-size: 12px;">{{ entry.split(':')[0] }}</div>
            <div class="row-sub" style="margin-top: 2px;">
              <span class="badge-pill blue" style="font-size: 10px; padding: 2px 6px;">{{ getMode(entry) }}</span>
            </div>
          </div>
        </div>
        <div style="display: flex; align-items: center; gap: 6px; flex-shrink: 0; margin-left: 8px;">
          <button class="btn-md3 btn-md3-primary" style="padding: 6px 12px; font-size: 11px;" @click="launch(entry)">
            <Icons name="rocket" :size="14" />
            <span>Launch</span>
          </button>
          <button class="btn-md3 btn-md3-danger" style="padding: 6px 10px; font-size: 11px;" @click="remove(entry)">
            <Icons name="trash" :size="14" />
          </button>
        </div>
      </div>
    </div>

    <!-- Add Application Box -->
    <div class="md3-card" style="margin-top: 16px;">
      <div style="font-size: 12px; font-weight: 600; color: var(--on-surface); margin-bottom: 8px;">Add game</div>
      <div style="display: flex; gap: 6px;">
        <input v-model="newPkg" type="text" class="input-md3" style="flex: 1;" placeholder="com.package.name or com.pkg:TOUCH" @keydown.enter="add">
        <button class="btn-md3 btn-md3-secondary" @click="add">
          <Icons name="plus" :size="16" />
          <span>Add</span>
        </button>
        <button class="btn-md3 btn-md3-primary" @click="showPicker = true">
          <Icons name="device" :size="16" />
          <span>Select app</span>
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
