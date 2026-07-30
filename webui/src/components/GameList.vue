<template>
  <div class="card">
    <div class="card-title">
      <span>Game list</span>
      <span class="badge">{{ store.games.length }} {{ store.games.length === 1 ? 'game' : 'games' }}</span>
    </div>

    <!-- Game entries -->
    <div class="game-scroll">
      <div v-if="store.games.length === 0" class="empty">No games configured</div>
      <div v-for="entry in store.games" :key="entry" class="game-item">
        <div class="game-left">
          <img :src="iconFor(entry)" class="game-icon" @error="e => e.target.style.visibility = 'hidden'" loading="lazy">
          <span>{{ entry }}</span>
        </div>
        <div class="game-actions">
          <button class="btn btn-sm btn-launch" @click="launch(entry)">Launch</button>
          <button class="btn btn-sm btn-danger" @click="remove(entry)">Remove</button>
        </div>
      </div>
    </div>

    <!-- Add row -->
    <div class="add-row">
      <input v-model="newPkg" type="text" placeholder="com.example.game" @keydown.enter="add">
      <button class="btn btn-sm" @click="add">Add</button>
      <button class="btn btn-sm btn-launch" @click="showPicker = true">Select app</button>
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

const store = useHyperStore()
const toast = inject('toast')
const newPkg = ref('')
const showPicker = ref(false)

function iconFor(entry) {
  return getIconUrl(entry.split(':')[0].trim())
}

async function add() {
  if (!newPkg.value.trim()) return
  const msg = await store.addGame(newPkg.value.trim())
  if (msg) toast(msg)
  newPkg.value = ''
}

async function remove(entry) {
  const msg = await store.removeGame(entry)
  if (msg) toast(msg)
}

async function launch(entry) {
  const msg = await store.launchGame(entry)
  if (msg) toast(msg)
}

async function onPick(pkg) {
  showPicker.value = false
  const msg = await store.addGame(pkg)
  if (msg) toast(msg)
}
</script>

<style scoped>
.game-scroll {
  max-height: 320px;
  overflow-y: auto;
  margin-top: 4px;
}
.game-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: var(--bg);
  border: 1px solid var(--border);
  padding: 5px 8px;
  border-radius: var(--radius-sm);
  margin-bottom: 4px;
  font-size: 11px;
  font-family: var(--mono);
  gap: 8px;
}
.game-left {
  display: flex;
  align-items: center;
  gap: 8px;
  overflow: hidden;
  flex: 1;
  min-width: 0;
}
.game-icon {
  width: 18px; height: 18px;
  border-radius: 3px;
  flex: none;
  object-fit: cover;
  background: var(--border);
}
.game-left span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--text);
}
.game-actions { display: flex; gap: 4px; flex: none; }
.add-row { display: flex; gap: 6px; margin-top: 8px; }
.add-row input { flex: 1; }
.empty {
  font-size: 11px;
  color: var(--text-muted);
  text-align: center;
  padding: 12px 0;
}
</style>
