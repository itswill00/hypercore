<template>
  <Teleport to="body">
    <div class="modal-backdrop" @click.self="$emit('close')">
      <div class="modal-card">
        <!-- Header -->
        <div style="padding: 14px 16px; border-bottom: 1px solid var(--surface-container-high); display: flex; align-items: center; justify-content: space-between;">
          <div>
            <div style="font-size: 14px; font-weight: 600; color: var(--on-surface);">Select Application</div>
            <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 1px;">Installed system & user packages</div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 11px;" @click="$emit('close')">Close</button>
        </div>

        <!-- Filter -->
        <div style="padding: 10px 14px; border-bottom: 1px solid var(--surface-container-high); background: var(--surface-container-low);">
          <input
            v-model="query"
            type="text"
            class="input-md3"
            placeholder="Search app name or package..."
            ref="searchInput"
          >
        </div>

        <!-- List Body -->
        <div style="padding: 8px; overflow-y-auto; flex: 1; scrollbar-width: none;">
          <div v-if="loading" style="padding: 32px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
            Scanning installed packages...
          </div>
          <div v-else-if="filtered.length === 0" style="padding: 32px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
            No matching applications found
          </div>
          <div
            v-for="app in filtered"
            :key="app.pkg"
            class="md3-list-row clickable"
            style="border-radius: 12px; margin-bottom: 4px; border-bottom: none;"
            @click="$emit('pick', app.pkg)"
          >
            <div class="row-left">
              <img
                :src="iconFor(app.pkg)"
                style="width: 32px; height: 32px; border-radius: 10px; background: var(--surface-container-highest); object-fit: cover; flex-shrink: 0;"
                @error="e => e.target.style.visibility = 'hidden'"
                loading="lazy"
              >
              <div class="row-meta">
                <div class="row-title" style="font-size: 12px;">{{ app.name }}</div>
                <div class="row-sub" style="font-family: var(--font-mono); font-size: 10px; opacity: 0.75;">{{ app.pkg }}</div>
              </div>
            </div>
            <button class="btn-md3 btn-md3-primary" style="padding: 4px 10px; font-size: 11px; flex-shrink: 0; margin-left: 8px;" @click.stop="$emit('pick', app.pkg)">
              Add
            </button>
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

function iconFor(pkg) {
  return getIconUrl(pkg)
}

onMounted(async () => {
  try {
    apps.value = await listInstalledApps()
  } catch {
    apps.value = []
  }
  loading.value = false
  await nextTick()
  if (searchInput.value) searchInput.value.focus()
})
</script>
