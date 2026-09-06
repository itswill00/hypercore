<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">Detected Games</div>
        <div class="page-header-sub">Installed games configured for automated profile switching</div>
      </div>
      <div style="display: flex; align-items: center; gap: 8px;">
        <button
          class="btn-md3 btn-md3-secondary btn-md3-sm"
          :disabled="scanning"
          @click="onAutoDetect"
          title="Scan device for installed games"
        >
          <Icons name="search" :size="13" />
          <span>{{ scanning ? 'Scanning...' : 'Auto-detect' }}</span>
        </button>
        <button class="btn-md3 btn-md3-primary btn-md3-sm" @click="showPicker = true">
          <Icons name="plus" :size="14" />
          <span>Add game</span>
        </button>
      </div>
    </div>

    <div class="content-area">
      <GameList @open-picker="showPicker = true" />
    </div>

    <AppPicker v-if="showPicker" @close="showPicker = false" @pick="onPick" />
  </div>
</template>

<script setup>
import { ref, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import GameList from '@/components/GameList.vue'
import AppPicker from '@/components/AppPicker.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')
const showPicker = ref(false)
const scanning = ref(false)

async function onAutoDetect() {
  if (scanning.value) return
  scanning.value = true
  try {
    const msg = await store.autoDetectGames()
    if (msg && toast) toast(msg)
  } finally {
    scanning.value = false
  }
}

async function onPick(pkg, profile = 'GAMING') {
  showPicker.value = false
  const msg = await store.addGame(pkg, profile)
  if (msg && toast) toast(msg)
}
</script>
