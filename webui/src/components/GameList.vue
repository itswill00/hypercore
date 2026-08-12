<template>
  <div>
    
    <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 12px;">
      <input
        v-model="search"
        type="text"
        class="input-md3"
        placeholder="Search configured games..."
        style="flex: 1; border-radius: 20px; padding: 6px 14px; font-size: 11px;"
      >
      <span class="badge-pill" style="flex-shrink: 0; padding: 4px 10px;">
        {{ displayGames.length }} {{ displayGames.length === 1 ? 'app' : 'apps' }}
      </span>
    </div>

    <div class="md3-list-group">
      
      <div v-if="displayGames.length === 0 && !loading" style="padding: 32px 16px; text-align: center;">
        <div class="icon-badge secondary" style="width: 44px; height: 44px; margin: 0 auto 10px auto;">
          <Icons name="rocket" :size="20" />
        </div>
        <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">
          {{ search ? 'No matching games found' : 'No games configured' }}
        </div>
        <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 2px; max-width: 260px; margin-left: auto; margin-right: auto;">
          Add installed games to automatically trigger high performance profiles when playing
        </div>
        <button class="btn-md3 btn-md3-primary btn-md3-sm" style="margin-top: 14px;" @click="$emit('open-picker')">
          <Icons name="plus" :size="14" />
          <span>Add game</span>
        </button>
      </div>

      <div v-else-if="loading" style="padding: 24px 16px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
        Scanning configured games...
      </div>

      <TransitionGroup name="list-fade">
        <div
          v-for="item in displayGames"
          :key="item.pkg"
          class="md3-list-row game-list-item"
          style="padding: 10px 14px;"
        >
          <div class="row-left" style="gap: 10px; min-width: 0;">
            <div class="icon-frame">
              <img
                v-if="iconFor(item.pkg) && !brokenIcons[item.pkg]"
                :src="iconFor(item.pkg)"
                style="width: 36px; height: 36px; border-radius: 10px; object-fit: cover;"
                @error="brokenIcons[item.pkg] = true"
                loading="lazy"
              >
              <div
                v-else
                class="icon-fallback"
                :style="{ background: getAppGradient(item.name) }"
              >
                {{ getAppInitial(item.name) }}
              </div>
            </div>
            <div class="row-meta" style="min-width: 0;">
              <div
                class="row-title"
                style="font-size: 13px; font-weight: 600; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;"
                :title="item.name"
              >
                {{ item.name }}
              </div>
              <div
                class="row-sub"
                style="font-family: var(--font-mono); font-size: 10px; opacity: 0.75; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;"
                :title="item.pkg"
              >
                {{ item.pkg }}
              </div>
            </div>
          </div>

          <div style="display: flex; align-items: center; gap: 8px; flex-shrink: 0; margin-left: 6px;">
            
            <select
              v-model="item.profile"
              @change="changeProfile(item.pkg, item.profile)"
              class="select-md3"
            >
              <option value="GAMING">Gaming</option>
              <option value="GAMING_MOBA">Gaming (MOBA)</option>
              <option value="INTERACTIVE">Interactive</option>
              <option value="SLEEP">Saver</option>
            </select>

            <button
              class="btn-md3 btn-md3-primary btn-md3-sm"
              @click="launch(item.pkg)"
            >
              <Icons name="rocket" :size="13" />
              <span>Launch</span>
            </button>
            <button
              class="btn-md3 btn-md3-danger btn-icon-only btn-md3-sm"
              title="Remove game"
              @click="remove(item.pkg)"
            >
              <Icons name="trash" :size="13" />
            </button>
          </div>
        </div>
      </TransitionGroup>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { listInstalledApps, getIconUrl, formatPkgName } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

defineEmits(['open-picker'])

const store = useHyperStore()
const toast = inject('toast')
const search = ref('')
const installedMap = ref({})
const iconMap = ref({})
const brokenIcons = ref({})
const loading = ref(true)

async function loadInstalled() {
  loading.value = true
  try {
    const list = await listInstalledApps()
    const nameMap = {}
    const iMap = {}

    list.forEach(a => {
      nameMap[a.pkg] = a.name
      if (a.icon) iMap[a.pkg] = a.icon
    })

    if (store.games && store.games.length > 0 && typeof ksu !== 'undefined' && typeof ksu.getPackagesInfo === 'function') {
      try {
        const pkgs = store.games.map(g => g.pkg)
        const infos = JSON.parse(ksu.getPackagesInfo(JSON.stringify(pkgs)))
        if (Array.isArray(infos)) {
          infos.forEach(info => {
            if (info && info.packageName) {
              if (info.appLabel) nameMap[info.packageName] = info.appLabel
              if (info.icon || info.appIcon) iMap[info.packageName] = info.icon || info.appIcon
            }
          })
        }
      } catch {}
    }

    installedMap.value = nameMap
    iconMap.value = iMap
  } catch {}
  loading.value = false
}

onMounted(() => {
  loadInstalled()
})

const displayGames = computed(() => {
  if (!store.games) return []

  const parsed = store.games.map(item => {
    const appName = installedMap.value[item.pkg] || formatPkgName(item.pkg)

    return {
      pkg: item.pkg,
      profile: item.profile,
      name: appName
    }
  })

  if (search.value.trim()) {
    const q = search.value.toLowerCase()
    return parsed.filter(item =>
      item.name.toLowerCase().includes(q) || item.pkg.toLowerCase().includes(q)
    )
  }

  return parsed
})

function iconFor(pkg) {
  if (iconMap.value[pkg]) return iconMap.value[pkg]
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

async function remove(pkg) {
  const msg = store.removeGame(pkg)
  if (msg && toast) toast(msg)
}

async function launch(pkg) {
  const msg = store.launchGame(pkg)
  if (msg && toast) toast(msg)
}

async function changeProfile(pkg, profile) {
  const msg = store.updateGameProfile(pkg, profile)
  if (msg && toast) toast(msg)
}
</script>

<style scoped>
.icon-frame {
  width: 36px;
  height: 36px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.icon-fallback {
  width: 36px;
  height: 36px;
  border-radius: 10px;
  color: #ffffff;
  font-weight: 700;
  font-size: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.4);
}

.select-md3 {
  background: var(--surface-container-high);
  color: var(--on-surface);
  border: 1px solid var(--surface-container-highest);
  border-radius: 8px;
  padding: 5px 8px;
  font-size: 11px;
  outline: none;
  cursor: pointer;
  font-family: inherit;
  transition: all 0.2s ease;
}

.select-md3:focus {
  border-color: var(--primary);
}
</style>
