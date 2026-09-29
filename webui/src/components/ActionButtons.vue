<template>
  <div>
    <div class="section-title">Quick Actions</div>
    <div class="md3-list-group">
      <div class="md3-list-row clickable" :class="{ 'disabled': store.loading }" @click="openHud">
        <div class="row-left">
          <div class="icon-badge" :class="hudStore.config.visible ? '' : 'secondary'">
            <Icons name="moon" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">HyperMoon HUD</div>
            <div class="row-sub">{{ hudSub }}</div>
          </div>
        </div>
        <div class="row-val" @click.stop="toggleHud">
          <label class="md3-switch" style="pointer-events: none;">
            <input type="checkbox" :checked="hudStore.config.visible" />
            <span class="md3-switch-track">
              <span class="md3-switch-thumb"></span>
            </span>
          </label>
        </div>
      </div>

      <div class="md3-list-row clickable" :class="{ 'disabled': store.loading }" @click="flush">
        <div class="row-left">
          <div class="icon-badge">
            <Icons name="memory" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Clear RAM cache</div>
            <div class="row-sub">Compact memory &amp; drop caches</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" :class="{ 'disabled': store.loading }" @click="openBugreport">
        <div class="row-left">
          <div class="icon-badge secondary">
            <Icons name="logs" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Bug Report</div>
            <div class="row-sub">Collect logs, sysfs &amp; device snapshot</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" :class="{ 'disabled': store.loading }" @click="restart">
        <div class="row-left">
          <div class="icon-badge tertiary">
            <Icons name="refresh" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Restart daemon</div>
            <div class="row-sub">Relaunch daemon service</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" @click="addShortcut">
        <div class="row-left">
          <div class="icon-badge">
            <Icons name="star" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Create shortcut</div>
            <div class="row-sub">Add home screen launcher shortcut</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>
    </div>

    <BugreportModal :show="showBugreport" @close="showBugreport = false" />
  </div>
</template>

<script setup>
import { ref, computed, inject, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useHyperStore } from '@/stores/hyper'
import { useHyperMoonStore } from '@/stores/hypermoon'
import Icons from '@/components/icons/Icons.vue'
import BugreportModal from '@/components/BugreportModal.vue'

const store = useHyperStore()
const hudStore = useHyperMoonStore()
const router = useRouter()
const toast = inject('toast')

const showBugreport = ref(false)

const hudSub = computed(() => {
  if (!hudStore.config.visible) return 'In-game overlay designer'
  if (hudStore.stats.fps && hudStore.stats.fps !== '--') return `${hudStore.stats.fps} FPS on screen`
  return hudStore.config.auto_gaming ? 'Auto-shows in games' : 'Overlay active'
})

onMounted(() => {
  hudStore.init()
})

function openHud() {
  router.push('/hud')
}

function toggleHud() {
  if (store.loading) return
  hudStore.toggleMaster(!hudStore.config.visible)
}

async function flush() {
  if (store.loading) return
  if (toast) toast('Clearing RAM cache...')
  const msg = await store.flushRam()
  if (toast) toast(msg)
}

async function restart() {
  if (store.loading) return
  if (toast) toast('Restarting daemon...')
  const msg = await store.restartDaemon()
  if (toast) toast(msg)
}

function openBugreport() {
  if (store.loading) return
  showBugreport.value = true
}

function addShortcut() {
  const msg = store.createShortcut()
  if (toast) toast(msg)
}
</script>

<style scoped>
.disabled {
  opacity: 0.5;
  pointer-events: none;
}
</style>
