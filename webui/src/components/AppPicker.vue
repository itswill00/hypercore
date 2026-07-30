<template>
  <Teleport to="body">
    <div class="modal-backdrop" @click.self="$emit('close')">
      <div class="modal-card flex flex-col max-h-[80vh] overflow-hidden p-0">
        <!-- Header -->
        <div class="p-4 border-b border-surface-container-high flex items-center justify-between">
          <div>
            <h2 class="text-base font-semibold text-on-surface">Select Application</h2>
            <p class="text-xs text-on-surface-variant">Installed system and user packages</p>
          </div>
          <button class="md3-btn md3-btn-secondary py-1.5 px-3 text-xs" @click="$emit('close')">Close</button>
        </div>

        <!-- Filter -->
        <div class="p-3 border-b border-surface-container-high bg-surface-container-low">
          <input
            v-model="query"
            type="text"
            class="md3-input"
            placeholder="Search by app name or package..."
            ref="searchInput"
          >
        </div>

        <!-- List Body -->
        <div class="p-2 overflow-y-auto flex-1 scrollbar-hidden">
          <div v-if="loading" class="p-8 text-center text-xs text-on-surface-variant">
            Scanning installed packages...
          </div>
          <div v-else-if="filtered.length === 0" class="p-8 text-center text-xs text-on-surface-variant">
            No matching applications found
          </div>
          <div
            v-for="app in filtered"
            :key="app.pkg"
            class="md3-list-item clickable rounded-xl mb-1.5 border border-transparent hover:border-surface-bright"
            @click="$emit('pick', app.pkg)"
          >
            <div class="flex items-center gap-3 overflow-hidden min-w-0 flex-1">
              <img
                :src="iconFor(app.pkg)"
                class="w-9 h-9 rounded-xl bg-surface-container-highest object-cover shrink-0"
                @error="e => e.target.style.visibility = 'hidden'"
                loading="lazy"
              >
              <div class="min-w-0 flex-1">
                <div class="item-title truncate text-xs font-medium">{{ app.name }}</div>
                <div class="item-sub truncate text-[11px] font-mono opacity-70">{{ app.pkg }}</div>
              </div>
            </div>
            <button class="md3-btn md3-btn-primary py-1 px-3 text-xs shrink-0 ml-2" @click.stop="$emit('pick', app.pkg)">
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
