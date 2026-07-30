<template>
  <Teleport to="body">
    <div class="overlay" @click.self="$emit('close')">
      <div class="modal">
        <div class="modal-head">
          <span>Installed applications</span>
          <button class="btn btn-sm" @click="$emit('close')">Close</button>
        </div>
        <div class="modal-filter">
          <input v-model="query" type="text" placeholder="Search app or package..." ref="searchInput">
        </div>
        <div class="modal-body">
          <div v-if="loading" class="empty">Loading applications...</div>
          <div v-else-if="filtered.length === 0" class="empty">No matching applications</div>
          <div v-for="app in filtered" :key="app.pkg" class="app-row" @click="$emit('pick', app.pkg)">
            <div class="app-left">
              <img :src="iconFor(app.pkg)" class="app-icon" @error="e => e.target.style.visibility = 'hidden'" loading="lazy">
              <div class="app-meta">
                <div class="app-name">{{ app.name }}</div>
                <div class="app-pkg">{{ app.pkg }}</div>
              </div>
            </div>
            <button class="btn btn-sm" @click.stop="$emit('pick', app.pkg)">Add</button>
          </div>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<script setup>
import { ref, computed, onMounted, nextTick } from 'vue'
import { listInstalledApps, getIconUrl } from '@/helpers/shell'

defineEmits(['close', 'pick'])

const apps = ref([])
const query = ref('')
const loading = ref(true)
const searchInput = ref(null)

const filtered = computed(() => {
  if (!query.value.trim()) return apps.value
  const q = query.value.toLowerCase()
  return apps.value.filter(a =>
    a.name.toLowerCase().includes(q) || a.pkg.toLowerCase().includes(q)
  )
})

function iconFor(pkg) { return getIconUrl(pkg) }

onMounted(async () => {
  try {
    apps.value = await listInstalledApps()
  } catch { apps.value = [] }
  loading.value = false
  await nextTick()
  if (searchInput.value) searchInput.value.focus()
})
</script>

<style scoped>
.overlay {
  position: fixed;
  inset: 0;
  background: rgba(0,0,0,0.8);
  backdrop-filter: blur(6px);
  z-index: 9999;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
}
.modal {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  width: 100%;
  max-width: 460px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  animation: modalIn 0.2s ease;
}
@keyframes modalIn { from { opacity: 0; transform: scale(0.96); } to { opacity: 1; transform: scale(1); } }
.modal-head {
  padding: 12px 14px;
  border-bottom: 1px solid var(--border);
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
  font-weight: 600;
  color: #fff;
}
.modal-filter {
  padding: 8px 14px;
  border-bottom: 1px solid var(--border);
}
.modal-filter input {
  width: 100%;
  background: var(--bg);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 7px 10px;
  color: var(--text);
  font-size: 11px;
  font-family: var(--mono);
}
.modal-filter input:focus { outline: none; border-color: var(--accent); }
.modal-body {
  padding: 6px 8px;
  overflow-y: auto;
  flex: 1;
}
.app-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 8px;
  border-radius: var(--radius-sm);
  border-bottom: 1px solid var(--border);
  gap: 10px;
  cursor: pointer;
  transition: background 0.15s ease;
}
.app-row:last-child { border-bottom: none; }
.app-row:hover { background: rgba(255,255,255,0.03); }
.app-left {
  display: flex;
  align-items: center;
  gap: 8px;
  overflow: hidden;
  flex: 1;
  min-width: 0;
}
.app-icon {
  width: 28px; height: 28px;
  border-radius: var(--radius-sm);
  background: var(--border);
  flex: none;
  object-fit: cover;
}
.app-meta { overflow: hidden; min-width: 0; }
.app-name {
  font-size: 12px;
  font-weight: 500;
  color: #fff;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.app-pkg {
  font-size: 10px;
  color: var(--text-muted);
  font-family: var(--mono);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.empty {
  font-size: 11px;
  color: var(--text-muted);
  text-align: center;
  padding: 16px 0;
}
</style>
