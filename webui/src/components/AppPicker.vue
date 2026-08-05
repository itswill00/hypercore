<template>
  <Teleport to="body">
    <transition name="modal-fade" appear>
      <div class="modal-backdrop" @click.self="$emit('close')">
        <div class="modal-card md3-picker-card">
          
          <!-- Header -->
          <div class="picker-header">
            <div class="header-info">
              <div class="header-title-row">
                <div class="icon-badge primary" style="width: 28px; height: 28px;">
                  <Icons name="plus" :size="16" />
                </div>
                <div class="header-title">Add Application</div>
                <span class="badge-pill purple" style="font-size: 10px; padding: 2px 8px;">
                  {{ availableAppsCount }} available
                </span>
              </div>
              <div class="header-sub">Configure high-performance profiles for your apps</div>
            </div>
            <button class="btn-icon-close" title="Close" @click="$emit('close')">
              <Icons name="close" :size="16" />
            </button>
          </div>

          <!-- Search & Filter Controls -->
          <div class="picker-controls">
            <!-- Search Bar -->
            <div class="search-input-wrap">
              <Icons name="search" :size="16" class="search-icon" />
              <input
                v-model="query"
                type="text"
                class="picker-search-input"
                placeholder="Search app name or package ID (e.g. com.game)..."
                ref="searchInput"
                @keydown.enter="addCustom"
              >
              <button v-if="query" class="search-clear-btn" @click="query = ''" title="Clear">
                <Icons name="close" :size="14" />
              </button>
            </div>

            <!-- Segmented Category Tabs & Target Profile Selector -->
            <div class="control-row">
              <div class="segmented-tabs">
                <button
                  class="tab-btn"
                  :class="{ active: activeTab === 'installed' }"
                  @click="activeTab = 'installed'"
                >
                  Installed Apps
                </button>
                <button
                  class="tab-btn"
                  :class="{ active: activeTab === 'custom' }"
                  @click="activeTab = 'custom'"
                >
                  Custom Package
                </button>
              </div>

              <!-- Profile Choice Chips -->
              <div class="profile-chips">
                <span class="chips-label">Profile:</span>
                <button
                  v-for="p in profiles"
                  :key="p.value"
                  class="profile-chip"
                  :class="{ active: selectedProfile === p.value }"
                  @click="selectedProfile = p.value"
                >
                  {{ p.label }}
                </button>
              </div>
            </div>
          </div>

          <!-- Main Content Body -->
          <div class="picker-body">
            
            <!-- Tab 1: Installed Apps -->
            <template v-if="activeTab === 'installed'">
              <div v-if="loading" class="state-message">
                <div class="spinner-sm"></div>
                <span>Scanning installed applications...</span>
              </div>

              <div v-else-if="filteredApps.length === 0" class="state-message">
                <div class="icon-badge secondary" style="width: 40px; height: 40px; margin: 0 auto 10px auto;">
                  <Icons name="games" :size="18" />
                </div>
                <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">
                  {{ query ? 'No matching apps found' : 'All installed apps already added' }}
                </div>
                <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 4px; max-width: 280px; margin-left: auto; margin-right: auto;">
                  {{ query ? 'Try searching another name or switch to Custom Package tab' : 'You can enter a package ID manually via Custom Package tab' }}
                </div>
                <button v-if="query.trim()" class="btn-md3 btn-md3-primary" style="margin-top: 12px; font-size: 11px;" @click="addCustom">
                  <Icons name="plus" :size="14" />
                  <span>Add "{{ query.trim() }}"</span>
                </button>
              </div>

              <div v-else class="app-list-container">
                <div
                  v-for="app in filteredApps"
                  :key="app.pkg"
                  class="md3-list-row app-item-row"
                  @click="pickApp(app.pkg)"
                >
                  <div class="row-left">
                    <div class="icon-frame">
                      <img
                        v-if="(app.icon || iconFor(app.pkg)) && !brokenIcons[app.pkg]"
                        :src="app.icon || iconFor(app.pkg)"
                        style="width: 34px; height: 34px; border-radius: 10px; object-fit: cover;"
                        @error="brokenIcons[app.pkg] = true"
                        loading="lazy"
                      >
                      <div
                        v-else
                        class="icon-fallback"
                        :style="{ background: getAppGradient(app.name) }"
                      >
                        {{ getAppInitial(app.name) }}
                      </div>
                    </div>
                    <div class="row-meta">
                      <div class="row-title">{{ app.name }}</div>
                      <div class="row-sub">{{ app.pkg }}</div>
                    </div>
                  </div>

                  <button
                    class="btn-md3 btn-md3-primary btn-add-row"
                    @click.stop="pickApp(app.pkg)"
                  >
                    <Icons name="plus" :size="13" />
                    <span>Add</span>
                  </button>
                </div>
              </div>
            </template>

            <!-- Tab 2: Custom Package -->
            <template v-else-if="activeTab === 'custom'">
              <div class="custom-card">
                <div class="custom-card-title">Manual Package Registration</div>
                <div class="custom-card-desc">
                  Enter exact Android package identifier (e.g. <code>com.miHoYo.GenshinImpact</code>) to configure custom profiling rule.
                </div>

                <div class="custom-input-group">
                  <input
                    v-model="customPkg"
                    type="text"
                    class="input-md3"
                    placeholder="com.package.name"
                    @keydown.enter="submitCustom"
                  >
                  <button
                    class="btn-md3 btn-md3-primary"
                    :disabled="!customPkg.trim()"
                    @click="submitCustom"
                  >
                    <Icons name="plus" :size="14" />
                    <span>Add Package</span>
                  </button>
                </div>

                <div v-if="customPkg.trim()" class="custom-preview">
                  Target: <strong>{{ formattedCustomName }}</strong> ({{ selectedProfile }})
                </div>
              </div>
            </template>

          </div>
        </div>
      </div>
    </transition>
  </Teleport>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { listInstalledApps, getIconUrl, formatPkgName } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

const emit = defineEmits(['close', 'pick'])

const store = useHyperStore()
const apps = ref([])
const query = ref('')
const customPkg = ref('')
const activeTab = ref('installed')
const selectedProfile = ref('GAMING')
const loading = ref(true)
const brokenIcons = ref({})
const searchInput = ref(null)

const profiles = [
  { value: 'GAMING', label: 'Gaming' },
  { value: 'INTERACTIVE', label: 'Balanced' },
  { value: 'SLEEP', label: 'Saver' }
]

const existingPkgs = computed(() => {
  if (!store.games) return []
  return store.games.map(g => g.pkg)
})

const availableAppsCount = computed(() => {
  return apps.value.filter(a => !existingPkgs.value.includes(a.pkg)).length
})

const filteredApps = computed(() => {
  const available = apps.value.filter(a => !existingPkgs.value.includes(a.pkg))

  if (!query.value.trim()) return available
  const q = query.value.toLowerCase()
  return available.filter(a =>
    a.name.toLowerCase().includes(q) || a.pkg.toLowerCase().includes(q)
  )
})

const formattedCustomName = computed(() => {
  if (!customPkg.value.trim()) return ''
  return formatPkgName(customPkg.value.trim())
})

function iconFor(pkg) {
  return getIconUrl(pkg)
}

function getAppInitial(name) {
  if (!name) return '?'
  const words = name.trim().split(/\s+/)
  if (words.length >= 2) {
    return (words[0][0] + words[1][0]).toUpperCase()
  }
  return name.substring(0, 2).toUpperCase()
}

function getAppGradient(name) {
  let hash = 0
  for (let i = 0; i < (name || '').length; i++) {
    hash = name.charCodeAt(i) + ((hash << 5) - hash)
  }
  const h1 = Math.abs(hash) % 360
  const h2 = (h1 + 40) % 360
  return `linear-gradient(135deg, hsl(${h1}, 70%, 45%), hsl(${h2}, 75%, 35%))`
}

function pickApp(pkg) {
  if (!pkg) return
  emit('pick', pkg, selectedProfile.value)
}

function addCustom() {
  if (query.value.trim()) {
    pickApp(query.value.trim())
  }
}

function submitCustom() {
  if (customPkg.value.trim()) {
    pickApp(customPkg.value.trim())
  }
}

onMounted(async () => {
  try {
    apps.value = await listInstalledApps()
  } catch {
    apps.value = []
  }
  loading.value = false
})
</script>

<style scoped>
.md3-picker-card {
  width: 92%;
  max-width: 460px;
  max-height: 85vh;
  display: flex;
  flex-direction: column;
  background: var(--surface-container-high, #1e1e24);
  border: 1px solid var(--surface-container-highest, #2b2b36);
  border-radius: 24px;
  overflow: hidden;
  box-shadow: 0 16px 32px rgba(0, 0, 0, 0.4);
}

.picker-header {
  padding: 16px 18px 12px 18px;
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  border-bottom: 1px solid var(--surface-container-highest, #2b2b36);
}

.header-title-row {
  display: flex;
  align-items: center;
  gap: 10px;
}

.header-title {
  font-size: 15px;
  font-weight: 700;
  color: var(--on-surface, #ffffff);
}

.header-sub {
  font-size: 11px;
  color: var(--on-surface-variant, #a0a0b0);
  margin-top: 3px;
}

.btn-icon-close {
  background: var(--surface-container-highest, #2b2b36);
  color: var(--on-surface-variant, #a0a0b0);
  border: none;
  width: 30px;
  height: 30px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s ease;
}

.btn-icon-close:hover {
  background: var(--surface-container, #363646);
  color: var(--on-surface, #ffffff);
}

.picker-controls {
  padding: 12px 16px;
  background: var(--surface-container-low, #16161b);
  border-bottom: 1px solid var(--surface-container-highest, #2b2b36);
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.search-input-wrap {
  position: relative;
  display: flex;
  align-items: center;
}

.search-icon {
  position: absolute;
  left: 12px;
  color: var(--on-surface-variant, #a0a0b0);
  pointer-events: none;
}

.picker-search-input {
  width: 100%;
  background: var(--surface-container-high, #1e1e24);
  color: var(--on-surface, #ffffff);
  border: 1px solid var(--surface-container-highest, #2b2b36);
  border-radius: 20px;
  padding: 8px 36px 8px 36px;
  font-size: 11px;
  outline: none;
  transition: border-color 0.2s ease;
}

.picker-search-input:focus {
  border-color: var(--primary, #a855f7);
}

.search-clear-btn {
  position: absolute;
  right: 10px;
  background: transparent;
  border: none;
  color: var(--on-surface-variant, #a0a0b0);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 4px;
}

.control-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  flex-wrap: wrap;
}

.segmented-tabs {
  display: flex;
  background: var(--surface-container-high, #1e1e24);
  padding: 3px;
  border-radius: 14px;
  border: 1px solid var(--surface-container-highest, #2b2b36);
}

.tab-btn {
  background: transparent;
  border: none;
  color: var(--on-surface-variant, #a0a0b0);
  font-size: 10px;
  font-weight: 600;
  padding: 4px 10px;
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.tab-btn.active {
  background: var(--primary, #a855f7);
  color: #ffffff;
}

.profile-chips {
  display: flex;
  align-items: center;
  gap: 4px;
}

.chips-label {
  font-size: 10px;
  color: var(--on-surface-variant, #a0a0b0);
  margin-right: 2px;
}

.profile-chip {
  background: var(--surface-container-high, #1e1e24);
  color: var(--on-surface-variant, #a0a0b0);
  border: 1px solid var(--surface-container-highest, #2b2b36);
  font-size: 10px;
  padding: 3px 8px;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.profile-chip.active {
  background: rgba(168, 85, 247, 0.2);
  color: var(--primary, #c084fc);
  border-color: var(--primary, #a855f7);
  font-weight: 600;
}

.picker-body {
  padding: 10px 14px;
  overflow-y: auto;
  flex: 1;
  scrollbar-width: thin;
}

.state-message {
  padding: 36px 16px;
  text-align: center;
  font-size: 12px;
  color: var(--on-surface-variant, #a0a0b0);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.spinner-sm {
  width: 20px;
  height: 20px;
  border: 2px solid var(--surface-container-highest, #2b2b36);
  border-top-color: var(--primary, #a855f7);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  margin-bottom: 10px;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.app-list-container {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.app-item-row {
  border-radius: 14px;
  padding: 8px 12px;
  background: var(--surface-container, #22222a);
  border: 1px solid transparent;
  transition: all 0.2s ease;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.app-item-row:hover {
  background: var(--surface-container-high, #2a2a34);
  border-color: var(--surface-container-highest, #363646);
}

.icon-frame {
  width: 34px;
  height: 34px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.icon-fallback {
  width: 34px;
  height: 34px;
  border-radius: 10px;
  color: #ffffff;
  font-weight: 700;
  font-size: 11px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.4);
}

.row-title {
  font-size: 12px;
  font-weight: 600;
  color: var(--on-surface, #ffffff);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.row-sub {
  font-family: var(--font-mono, monospace);
  font-size: 10px;
  color: var(--on-surface-variant, #a0a0b0);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.btn-add-row {
  padding: 4px 10px;
  font-size: 10px;
  height: 28px;
  border-radius: 8px;
  flex-shrink: 0;
  margin-left: 8px;
}

.custom-card {
  padding: 16px;
  background: var(--surface-container, #22222a);
  border-radius: 16px;
  border: 1px solid var(--surface-container-highest, #2b2b36);
}

.custom-card-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface, #ffffff);
  margin-bottom: 4px;
}

.custom-card-desc {
  font-size: 11px;
  color: var(--on-surface-variant, #a0a0b0);
  margin-bottom: 14px;
  line-height: 1.4;
}

.custom-input-group {
  display: flex;
  gap: 8px;
}

.custom-preview {
  margin-top: 12px;
  font-size: 11px;
  color: var(--primary, #c084fc);
  padding: 8px 12px;
  background: rgba(168, 85, 247, 0.1);
  border-radius: 8px;
}
</style>
