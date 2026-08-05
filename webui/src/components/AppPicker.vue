<template>
  <Teleport to="body">
    <transition name="modal-fade" appear>
      <div class="modal-backdrop" @click.self="$emit('close')">
        <div class="modal-card picker-card">
          
          <!-- Header -->
          <div class="picker-header">
            <div class="header-left">
              <div class="icon-badge primary" style="width: 32px; height: 32px;">
                <Icons name="plus" :size="16" />
              </div>
              <div>
                <div class="header-title-wrap">
                  <span class="header-title">Add Application</span>
                  <span class="badge-pill purple">{{ availableAppsCount }} available</span>
                </div>
                <div class="header-sub">Configure performance profiles for your applications</div>
              </div>
            </div>
            <button class="btn-md3 btn-md3-secondary btn-icon-only" title="Close" @click="$emit('close')">
              <Icons name="close" :size="15" />
            </button>
          </div>

          <!-- Controls Bar -->
          <div class="picker-controls">
            <!-- Search Bar -->
            <div class="search-wrap">
              <Icons name="search" :size="15" class="search-icon" />
              <input
                v-model="query"
                type="text"
                class="input-md3 search-input"
                placeholder="Search app name or package ID..."
                ref="searchInput"
                @keydown.enter="addCustom"
              >
              <button v-if="query" class="clear-btn" @click="query = ''" title="Clear">
                <Icons name="close" :size="14" />
              </button>
            </div>

            <!-- Segmented Category Tabs & Profile Selector -->
            <div class="control-row">
              <div class="segment-container">
                <button
                  class="segment-btn"
                  :class="{ active: activeTab === 'installed' }"
                  @click="activeTab = 'installed'"
                >
                  Installed Apps
                </button>
                <button
                  class="segment-btn"
                  :class="{ active: activeTab === 'custom' }"
                  @click="activeTab = 'custom'"
                >
                  Custom Package
                </button>
              </div>

              <div class="profile-chip-group">
                <span class="profile-chip-label">Profile:</span>
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

          <!-- List Body -->
          <div class="picker-body">
            
            <template v-if="activeTab === 'installed'">
              <div v-if="loading" class="empty-state">
                <div class="spinner-sm"></div>
                <span>Scanning installed packages...</span>
              </div>

              <div v-else-if="filteredApps.length === 0" class="empty-state">
                <div class="icon-badge secondary" style="width: 40px; height: 40px; margin-bottom: 10px;">
                  <Icons name="games" :size="18" />
                </div>
                <div class="empty-title">
                  {{ query ? 'No matching apps found' : 'All installed apps already added' }}
                </div>
                <div class="empty-sub">
                  {{ query ? 'Search another app or enter package ID manually' : 'You can add package IDs manually via Custom Package tab' }}
                </div>
                <button v-if="query.trim()" class="btn-md3 btn-md3-primary" style="margin-top: 12px;" @click="addCustom">
                  <Icons name="plus" :size="14" />
                  <span>Add "{{ query.trim() }}"</span>
                </button>
              </div>

              <div v-else class="app-list">
                <div
                  v-for="app in filteredApps"
                  :key="app.pkg"
                  class="md3-list-row clickable app-row"
                  @click="pickApp(app.pkg)"
                >
                  <div class="row-left">
                    <div class="icon-frame">
                      <img
                        v-if="(app.icon || iconFor(app.pkg)) && !brokenIcons[app.pkg]"
                        :src="app.icon || iconFor(app.pkg)"
                        class="app-icon"
                        @error="brokenIcons[app.pkg] = true"
                        loading="lazy"
                      >
                      <div
                        v-else
                        class="icon-avatar"
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
                    class="btn-md3 btn-md3-primary btn-add"
                    @click.stop="pickApp(app.pkg)"
                  >
                    <Icons name="plus" :size="13" />
                    <span>Add</span>
                  </button>
                </div>
              </div>
            </template>

            <template v-else-if="activeTab === 'custom'">
              <div class="custom-card">
                <div class="custom-title">Manual Package Registration</div>
                <div class="custom-desc">
                  Enter exact Android package identifier (e.g. <code>com.miHoYo.GenshinImpact</code>) to register custom rule.
                </div>

                <div class="custom-input-wrap">
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
  { value: 'GAMING_MOBA', label: 'Gaming (MOBA)' },
  { value: 'INTERACTIVE', label: 'Interactive' },
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
.picker-card {
  width: 92%;
  max-width: 440px;
  max-height: 82vh;
  background: var(--surface-container);
  border: 1px solid var(--surface-container-high);
  border-radius: 20px;
}

.picker-header {
  padding: 14px 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-bottom: 1px solid var(--surface-container-high);
}

.header-left {
  display: flex;
  align-items: center;
  gap: 10px;
}

.header-title-wrap {
  display: flex;
  align-items: center;
  gap: 8px;
}

.header-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface);
}

.header-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 1px;
}

.picker-controls {
  padding: 10px 14px;
  background: var(--surface-container-low);
  border-bottom: 1px solid var(--surface-container-high);
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.search-wrap {
  position: relative;
  display: flex;
  align-items: center;
}

.search-icon {
  position: absolute;
  left: 10px;
  color: var(--on-surface-variant);
  pointer-events: none;
}

.search-input {
  padding-left: 32px;
  padding-right: 30px;
  border-radius: 10px;
  font-size: 11px;
}

.clear-btn {
  position: absolute;
  right: 8px;
  background: transparent;
  border: none;
  color: var(--on-surface-variant);
  cursor: pointer;
  padding: 2px;
  display: flex;
  align-items: center;
}

.control-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 6px;
  flex-wrap: wrap;
}

.profile-chip-group {
  display: flex;
  align-items: center;
  gap: 4px;
}

.profile-chip-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.profile-chip {
  background: var(--surface-container-high);
  color: var(--on-surface-variant);
  border: 1px solid transparent;
  font-size: 10px;
  font-weight: 500;
  padding: 3px 8px;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.15s ease;
}

.profile-chip.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-color: var(--outline-variant);
  font-weight: 600;
}

.picker-body {
  padding: 8px 12px;
  overflow-y: auto;
  flex: 1;
}

.empty-state {
  padding: 32px 16px;
  text-align: center;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.empty-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface);
}

.empty-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 2px;
}

.spinner-sm {
  width: 18px;
  height: 18px;
  border: 2px solid var(--surface-container-highest);
  border-top-color: var(--primary);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  margin-bottom: 8px;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.app-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.app-row {
  border-radius: 12px;
  padding: 8px 10px;
  background: var(--surface-container-low);
  border: 1px solid transparent;
}

.app-row:hover {
  background: var(--surface-container-high);
  border-color: var(--outline-variant);
}

.icon-frame {
  width: 32px;
  height: 32px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.app-icon {
  width: 32px;
  height: 32px;
  border-radius: 9px;
  object-fit: cover;
}

.icon-avatar {
  width: 32px;
  height: 32px;
  border-radius: 9px;
  color: #ffffff;
  font-weight: 700;
  font-size: 11px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
}

.btn-add {
  padding: 4px 10px;
  font-size: 11px;
  height: 28px;
  border-radius: 8px;
}

.custom-card {
  padding: 14px;
  background: var(--surface-container-low);
  border-radius: 14px;
  border: 1px solid var(--surface-container-high);
}

.custom-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface);
  margin-bottom: 3px;
}

.custom-desc {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-bottom: 12px;
}

.custom-input-wrap {
  display: flex;
  gap: 8px;
}

.custom-preview {
  margin-top: 10px;
  font-size: 11px;
  color: var(--on-primary-container);
  padding: 6px 10px;
  background: var(--primary-container);
  border-radius: 8px;
}
</style>
