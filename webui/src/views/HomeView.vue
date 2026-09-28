<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <div class="content-area">
      
      <!-- Home Hero Banner with banner.jpg Background -->
      <div class="md3-banner" :style="{ backgroundImage: `url(${bannerImg})`, backgroundSize: 'cover', backgroundPosition: 'center' }">
        <div class="banner-overlay"></div>
        <div class="row-left">
          <div class="icon-badge" style="width: 40px; height: 40px;">
            <Icons name="chip" :size="22" />
          </div>
          <div class="row-meta">
            <div class="row-title" style="font-size: 15px;">HyperCore</div>
            <div class="row-sub">Kernel Optimizer for MediaTek MT6789 Family</div>
          </div>
        </div>

        <div class="chips-row">
          <span class="badge-pill">Daemon: {{ store.isRunning ? `Active (PID ${store.daemonPid})` : 'Standby' }}</span>
          <span class="badge-pill">Profile: {{ store.activeProfile }}</span>
          <span class="badge-pill" v-if="store.cpuTemp > 0">CPU: {{ store.cpuTemp }}°C</span>
          <span class="badge-pill" v-if="store.batLevel">Batt: {{ store.batLevel }}%{{ store.batStatus === 'Charging' ? ' · Charging' : '' }}</span>
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
              <div class="row-sub">
                {{ hudStore.config.visible ? (hudStore.config.auto_gaming ? 'Overlay automatically shows during games' : 'Floating performance monitor is active') : 'Tap to customize or toggle in-game overlay' }}
              </div>
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
import { onMounted, inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { useHyperMoonStore } from '@/stores/hypermoon'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'
import bannerImg from '@/assets/banner.jpg'

const store = useHyperStore()
const hudStore = useHyperMoonStore()
const toast = inject('toast')

onMounted(() => {
  hudStore.init()
})

function onToggleHud() {
  hudStore.toggleMaster(!hudStore.config.visible)
}
</script>



