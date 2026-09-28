<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <div class="content-area">
      
      <!-- Live Status -->
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="row-meta">
              <div class="row-title" style="font-size: 17px;">{{ store.activeProfile }}</div>
              <div class="row-sub">PID {{ store.daemonPid || '—' }} · Up {{ store.uptime }}</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill">{{ store.thermalTier }}</span>
          </div>
        </div>

        <div class="md3-list-row">
          <div class="row-left" style="width: 100%;">
            <div class="stat-grid-2">
              <div class="stat-box">
                <div class="stat-lbl">CPU</div>
                <div class="stat-num">{{ store.cpuTemp > 0 ? `${store.cpuTemp}°C · ${store.cpuGov}` : '—' }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">GPU</div>
                <div class="stat-num">{{ store.gpuInfo }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">RAM</div>
                <div class="stat-num">{{ store.ramUsage }}</div>
              </div>
              <div class="stat-box">
                <div class="stat-lbl">Battery</div>
                <div class="stat-num">{{ store.batLevel ? `${store.batLevel}% · ${store.batStatus}` : store.batStatus }}</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- HyperMoon Performance HUD Card -->
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="$router.push('/hud')">
          <div class="row-left">
            <div class="icon-badge" :class="hudStore.config.visible ? '' : 'secondary'">
              <Icons name="moon" :size="20" />
            </div>
            <div class="row-meta">
              <div class="row-title">HyperMoon HUD</div>
              <div class="row-sub">{{ hudSub }}</div>
            </div>
          </div>
          <div class="row-val" @click.stop="onToggleHud">
            <span class="badge-pill" style="margin-right: 8px;">{{ hudStore.config.visible ? 'Active' : 'Off' }}</span>
            <label class="md3-switch" style="pointer-events: none; vertical-align: middle;">
              <input
                type="checkbox"
                :checked="hudStore.config.visible"
              />
              <span class="md3-switch-track">
                <span class="md3-switch-thumb"></span>
              </span>
            </label>
          </div>
        </div>
      </div>

      <ActionButtons />

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 8px 0 16px 0;">
        Inspired by encore @Rem01Gaming
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, inject, computed } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { useHyperMoonStore } from '@/stores/hypermoon'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const hudStore = useHyperMoonStore()
const toast = inject('toast')

const hudSub = computed(() => {
  if (!hudStore.config.visible) return 'Tap to customize or toggle in-game overlay'
  const fps = hudStore.stats.fps
  const fpsPart = (fps && fps !== '--') ? `${fps} FPS` : 'Live stats on screen'
  const ddrPart = (store.dvfsrcMhz > 0) ? ` · DDR ${(store.dvfsrcMhz / 1000).toFixed(2)} GHz` : ''
  const autoPart = hudStore.config.auto_gaming ? ' · auto-gaming' : ''
  return `${fpsPart}${ddrPart}${autoPart}`
})

onMounted(() => {
  hudStore.init()
})

function onToggleHud() {
  hudStore.toggleMaster(!hudStore.config.visible)
}
</script>



