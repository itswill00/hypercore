<template>
  <div>
    <!-- Search & Counter Header Bar -->
    <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 12px;">
      <input
        v-model="search"
        type="text"
        class="input-md3"
        placeholder="Search configured games..."
        style="flex: 1; border-radius: 20px; padding: 6px 14px; font-size: 11px;"
      >
      <span class="badge-pill purple" style="flex-shrink: 0; padding: 4px 10px;">
        {{ displayGames.length }} {{ displayGames.length === 1 ? 'app' : 'apps' }}
      </span>
    </div>

    <!-- Game List Group -->
    <div class="md3-list-group">
      <!-- Empty State -->
      <div v-if="displayGames.length === 0 && !loading" style="padding: 32px 16px; text-align: center;">
        <div class="icon-badge secondary" style="width: 44px; height: 44px; margin: 0 auto 10px auto;">
          <Icons name="rocket" :size="20" />
        </div>
        <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">
          {{ search ? 'No matching games found' : 'No configured games' }}
        </div>
        <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 2px; max-width: 240px; margin-left: auto; margin-right: auto;">
          Select installed games on your phone to trigger performance profiles automatically
        </div>
        <button class="btn-md3 btn-md3-primary" style="margin-top: 14px; font-size: 11px;" @click="$emit('open-picker')">
          <Icons name="plus" :size="14" />
          <span>Add game</span>
        </button>
      </div>

      <!-- Loading State -->
      <div v-else-if="loading" style="padding: 24px 16px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
        Checking configured games...
      </div>

      <!-- Installed Game Rows -->
      <div
        v-for="item in displayGames"
        :key="item.entry"
        class="md3-list-row"
        style="padding: 10px 14px;"
      >
        <div class="row-left" style="gap: 10px;">
          <div class="icon-frame">
            <img
              v-if="iconFor(item.pkg) && !brokenIcons[item.pkg]"
              :src="iconFor(item.pkg)"
              style="width: 36px; height: 36px; border-radius: 10px; object-fit: cover;"
              @error="brokenIcons[item.pkg] = true"
              loading="lazy"
            >
            <div v-else class="icon-fallback">
              <Icons name="rocket" :size="18" />
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
            <div style="display: flex; align-items: center; gap: 6px; margin-top: 2px;">
              <span
                class="row-sub"
                style="font-family: var(--font-mono); font-size: 10px; opacity: 0.75; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; max-width: 130px;"
                :title="item.pkg"
              >
                {{ item.pkg }}
              </span>
              <span
                class="badge-pill blue"
                style="font-size: 9px; padding: 2px 7px; cursor: pointer; user-select: none; flex-shrink: 0;"
                title="Click to cycle profile mode"
                @click="cycleMode(item.entry)"
              >
                {{ item.mode }} ⚡
              </span>
            </div>
          </div>
        </div>

        <div style="display: flex; align-items: center; gap: 6px; flex-shrink: 0; margin-left: 6px;">
          <button
            class="btn-md3 btn-md3-primary"
            style="padding: 5px 10px; font-size: 11px; height: 30px;"
            @click="launch(item.entry)"
          >
            <Icons name="rocket" :size="13" />
            <span>Launch</span>
          </button>
          <button
            class="btn-md3 btn-md3-danger btn-icon-only"
            style="width: 30px; height: 30px;"
            title="Remove app"
            @click="remove(item.entry)"
          >
            <Icons name="trash" :size="13" />
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { listInstalledApps, getIconUrl } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

defineEmits(['open-picker'])

const store = useHyperStore()
const toast = inject('toast')
const search = ref('')
const installedMap = ref({})
const brokenIcons = ref({})
const loading = ref(true)

async function loadInstalled() {
  loading.value = true
  try {
    const list = await listInstalledApps()
    const map = {}
    list.forEach(a => {
      map[a.pkg] = a.name
    })
    installedMap.value = map
  } catch {}
  loading.value = false
}

onMounted(() => {
  loadInstalled()
})

const displayGames = computed(() => {
  if (!store.games) return []

  const parsed = store.games.map(entry => {
    const parts = entry.split(':')
    const pkg = parts[0].trim()
    const mode = parts.length > 1 ? parts[1].trim().toUpperCase() : 'GAMING'
    const appName = installedMap.value[pkg]

    return {
      entry,
      pkg,
      mode,
      name: appName || pkg
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
  return getIconUrl(pkg)
}

function cycleMode(entry) {
  const parts = entry.split(':')
  const pkg = parts[0].trim()
  const currentMode = parts.length > 1 ? parts[1].trim().toUpperCase() : 'GAMING'
  const modes = ['GAMING', 'TOUCH', 'INTERACTIVE', 'SLEEP']
  const nextIdx = (modes.indexOf(currentMode) + 1) % modes.length
  const nextMode = modes[nextIdx]
  const newEntry = `${pkg}:${nextMode}`

  store.updateGameMode(entry, newEntry)
  if (toast) toast(`${installedMap.value[pkg] || pkg} mode set to ${nextMode}`)
}

async function remove(entry) {
  const msg = await store.removeGame(entry)
  if (msg && toast) toast(msg)
}

async function launch(entry) {
  const msg = await store.launchGame(entry)
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
  background: var(--surface-container-high);
  color: var(--primary);
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
