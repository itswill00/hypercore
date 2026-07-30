<template>
  <div>
    <!-- Search & Counter Header Bar -->
    <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 12px;">
      <input
        v-model="search"
        type="text"
        class="input-md3"
        placeholder="Search games..."
        style="flex: 1; border-radius: 20px; padding: 6px 14px; font-size: 11px;"
      >
      <span class="badge-pill purple" style="flex-shrink: 0; padding: 4px 10px;">
        {{ store.games.length }} {{ store.games.length === 1 ? 'app' : 'apps' }}
      </span>
    </div>

    <!-- Game List Group -->
    <div class="md3-list-group">
      <!-- Empty State -->
      <div v-if="store.games.length === 0" style="padding: 32px 16px; text-align: center;">
        <div class="icon-badge secondary" style="width: 44px; height: 44px; margin: 0 auto 10px auto;">
          <Icons name="rocket" :size="20" />
        </div>
        <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">No games configured</div>
        <div style="font-size: 11px; color: var(--on-surface-variant); margin-top: 2px; max-width: 240px; margin-left: auto; margin-right: auto;">
          Add package names to automatically boost performance on launch
        </div>
        <button class="btn-md3 btn-md3-primary" style="margin-top: 14px; font-size: 11px;" @click="$emit('open-picker')">
          <Icons name="plus" :size="14" />
          <span>Add first game</span>
        </button>
      </div>

      <!-- No Filter Match State -->
      <div v-else-if="filteredGames.length === 0" style="padding: 24px 16px; text-align: center; font-size: 12px; color: var(--on-surface-variant);">
        No apps matching "{{ search }}"
      </div>

      <!-- Game Item Rows -->
      <div
        v-for="entry in filteredGames"
        :key="entry"
        class="md3-list-row"
        style="padding: 10px 14px;"
      >
        <div class="row-left" style="gap: 10px;">
          <img
            :src="iconFor(entry)"
            style="width: 34px; height: 34px; border-radius: 10px; background: var(--surface-container-high); object-fit: cover; flex-shrink: 0;"
            @error="e => e.target.style.visibility = 'hidden'"
            loading="lazy"
          >
          <div class="row-meta" style="min-width: 0;">
            <div
              class="row-title"
              style="font-family: var(--font-mono); font-size: 11px; max-width: 170px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;"
              :title="entry.split(':')[0]"
            >
              {{ entry.split(':')[0] }}
            </div>
            <div style="margin-top: 2px;">
              <span
                class="badge-pill blue"
                style="font-size: 9px; padding: 2px 7px; cursor: pointer; user-select: none;"
                title="Click to cycle profile mode"
                @click="cycleMode(entry)"
              >
                {{ getMode(entry) }} ⚡
              </span>
            </div>
          </div>
        </div>

        <div style="display: flex; align-items: center; gap: 6px; flex-shrink: 0; margin-left: 6px;">
          <button
            class="btn-md3 btn-md3-primary"
            style="padding: 5px 10px; font-size: 11px; height: 30px;"
            @click="launch(entry)"
          >
            <Icons name="rocket" :size="13" />
            <span>Launch</span>
          </button>
          <button
            class="btn-md3 btn-md3-danger btn-icon-only"
            style="width: 30px; height: 30px;"
            title="Remove app"
            @click="remove(entry)"
          >
            <Icons name="trash" :size="13" />
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { getIconUrl } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

defineEmits(['open-picker'])

const store = useHyperStore()
const toast = inject('toast')
const search = ref('')

const filteredGames = computed(() => {
  if (!search.value.trim()) return store.games
  const q = search.value.toLowerCase()
  return store.games.filter(g => g.toLowerCase().includes(q))
})

function iconFor(entry) {
  return getIconUrl(entry.split(':')[0].trim())
}

function getMode(entry) {
  const parts = entry.split(':')
  return parts.length > 1 ? parts[1].trim().toUpperCase() : 'GAMING'
}

async function cycleMode(entry) {
  const parts = entry.split(':')
  const pkg = parts[0].trim()
  const currentMode = parts.length > 1 ? parts[1].trim().toUpperCase() : 'GAMING'
  const modes = ['GAMING', 'TOUCH', 'INTERACTIVE', 'SLEEP']
  const nextIdx = (modes.indexOf(currentMode) + 1) % modes.length
  const nextMode = modes[nextIdx]
  const newEntry = `${pkg}:${nextMode}`

  await store.removeGame(entry)
  const msg = await store.addGame(newEntry)
  if (toast) toast(`${pkg} mode set to ${nextMode}`)
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
