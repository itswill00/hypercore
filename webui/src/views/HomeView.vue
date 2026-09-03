<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">Dashboard</div>
        <div class="page-header-sub">Performance &amp; Status Overview</div>
      </div>
      <span class="badge-pill" style="font-size: 11px; padding: 4px 10px;">
        {{ store.moduleVersion || 'v6.4.5' }}
      </span>
    </div>

    <div class="content-area">
      
      <!-- Home Hero Banner Card with banner.jpg Background -->
      <div class="home-app-banner" :style="{ backgroundImage: `url(${bannerImg})` }">
        <div class="app-banner-overlay"></div>
        <div class="app-banner-content">
          <div class="banner-top-row">
            <div class="app-banner-icon">
              <Icons name="chip" :size="22" />
            </div>
            <div class="banner-title-group">
              <div class="banner-title-line">
                <span class="app-banner-title">HyperCore</span>
              </div>
              <div class="app-banner-sub">Universal Kernel Optimizer for MediaTek MT6789 Family</div>
            </div>
          </div>

          <div class="banner-stats-row">
            <div class="banner-chip">
              <span>Daemon: {{ store.isRunning ? `Active (PID ${store.daemonPid})` : 'Standby' }}</span>
            </div>
            <div class="banner-chip">
              <span>Profile: {{ store.activeProfile }}</span>
            </div>
            <div class="banner-chip">
              <span>Thermal: {{ store.thermalTier }}</span>
            </div>
            <div class="banner-chip" v-if="store.cpuTemp > 0">
              <span>CPU: {{ store.cpuTemp }}°C{{ store.cpuGov && store.cpuGov !== '—' ? ` · ${store.cpuGov}` : '' }}</span>
            </div>
            <div class="banner-chip" v-if="store.gpuInfo && store.gpuInfo !== '—'">
              <span>GPU: {{ store.gpuInfo }}</span>
            </div>
            <div class="banner-chip" v-if="store.ramUsage && store.ramUsage !== '—'">
              <span>RAM: {{ store.ramUsage }}</span>
            </div>
            <div class="banner-chip" v-if="store.batStatus === 'Charging'">
              <span>{{ store.thermalGuardState }}</span>
            </div>
            <div class="banner-chip" v-if="store.uptime && store.uptime !== '—'">
              <span>Up: {{ store.uptime }}</span>
            </div>
          </div>
        </div>
      </div>

      <ActionButtons />

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 12px 0 20px 0;">
        Inspired by encore @Rem01Gaming
      </div>
    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'
import bannerImg from '@/assets/banner.jpg'

const store = useHyperStore()
</script>

<style scoped>

.home-app-banner {
  position: relative;
  border-radius: 18px;
  overflow: hidden;
  margin-bottom: 16px;
  border: 1px solid var(--surface-container-highest);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
  background-image: url('banner.jpg'), url('./banner.jpg'), url('../banner.jpg');
  background-size: cover;
  background-position: center;
}

.app-banner-overlay {
  position: absolute;
  inset: 0;
  background: linear-gradient(135deg, rgba(16, 17, 22, 0.94) 0%, rgba(16, 17, 22, 0.82) 65%, rgba(16, 17, 22, 0.65) 100%);
  backdrop-filter: blur(2px);
  z-index: 1;
}

.app-banner-content {
  position: relative;
  z-index: 2;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.banner-top-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.app-banner-icon {
  width: 44px;
  height: 44px;
  border-radius: 12px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}

.banner-title-group {
  min-width: 0;
  flex: 1;
}

.banner-title-line {
  display: flex;
  align-items: center;
  gap: 8px;
}

.app-banner-title {
  font-size: 18px;
  font-weight: 800;
  color: var(--on-surface);
  letter-spacing: -0.3px;
}

.app-banner-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 2px;
  line-height: 1.4;
}

.banner-stats-row {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  padding-top: 6px;
  border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.banner-chip {
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  font-size: 10px;
  font-weight: 500;
  padding: 3px 9px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.08);
  color: var(--on-surface);
  border: 1px solid rgba(255, 255, 255, 0.12);
  display: flex;
  align-items: center;
  gap: 6px;
  backdrop-filter: blur(4px);
}

</style>

