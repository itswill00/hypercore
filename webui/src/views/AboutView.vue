<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">About</div>
        <div class="page-header-sub">Module information &amp; contributors</div>
      </div>
      <span class="badge-pill purple" style="font-size: 11px; padding: 4px 10px;">
        {{ store.moduleVersion || 'v4.3' }}
      </span>
    </div>

    <div class="content-area">
      
      <!-- Module Identity Hero Banner Card with banner.jpg Background -->
      <div class="about-app-banner">
        <div class="app-banner-overlay"></div>
        <div class="app-banner-content">
          <div class="banner-top-row">
            <div class="app-banner-icon">
              <Icons name="chip" :size="22" />
            </div>
            <div class="banner-title-group">
              <div class="banner-title-line">
                <span class="app-banner-title">HyperCore</span>
                <span class="badge-pill purple">{{ store.moduleVersion || 'v4.3' }}</span>
              </div>
              <div class="app-banner-sub">Universal Kernel Optimizer for MediaTek Helio G99 Ultra</div>
            </div>
          </div>

          <div class="banner-stats-row">
            <div class="banner-chip">
              <span class="chip-dot" :class="store.isRunning ? 'active' : ''"></span>
              <span>{{ store.isRunning ? `Daemon Active (PID ${store.daemonPid})` : 'Daemon Standby' }}</span>
            </div>
            <div class="banner-chip">
              <span>Profile: {{ store.activeProfile }}</span>
            </div>
            <div class="banner-chip">
              <span>Thermal: {{ store.thermalTier }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- System Specifications Group -->
      <div class="section-title">Specifications</div>
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="chip" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Target Platform</div>
              <div class="row-sub">MediaTek MT6789 (Helio G99 Ultra)</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill purple">MT6789</span>
          </div>
        </div>

        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="cpu" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Kernel Engine</div>
              <div class="row-sub">Linux {{ store.kernelVersion ? store.kernelVersion.split('-')[0] : '5.10.x' }} (aarch64)</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill green">EAS / QoS</span>
          </div>
        </div>

        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="zap" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">IPC Daemon</div>
              <div class="row-sub">Zero-overhead C Native Service</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill green">0ms IPC</span>
          </div>
        </div>
      </div>

      <!-- Developer & Inspiration Group -->
      <div class="section-title">Developer &amp; Maintainer</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://github.com/itswill00')">
          <div class="row-left">
            <div class="icon-badge primary">
              <Icons name="pictogram-user" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@itswill00</div>
              <div class="row-sub">Lead Developer &amp; Architect</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 10px;" @click.stop="openExternal('https://t.me/noticesa')">
            Telegram
          </button>
        </div>

        <div class="md3-list-row clickable" @click="openExternal('https://github.com/Rem01Gaming')">
          <div class="row-left">
            <div class="icon-badge secondary">
              <Icons name="pictogram-user" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@Rem01Gaming</div>
              <div class="row-sub">Inspiration &amp; Architecture Concepts (encore)</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 10px;" @click.stop="openExternal('https://github.com/Rem01Gaming')">
            GitHub
          </button>
        </div>
      </div>

      <!-- Quality Assurance Group -->
      <div class="section-title">Testing Team</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://t.me/Rafzzz182')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="pictogram-user" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@Rafzzz182</div>
              <div class="row-sub">Hardware &amp; Game Testing</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 10px;" @click.stop="openExternal('https://t.me/Rafzzz182')">
            Telegram
          </button>
        </div>

        <div class="md3-list-row clickable" @click="openExternal('https://t.me/anotherside551')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="pictogram-female" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@anotherside551</div>
              <div class="row-sub">HyperOS &amp; Custom ROM Testing</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 10px;" @click.stop="openExternal('https://t.me/anotherside551')">
            Telegram
          </button>
        </div>
      </div>

      <!-- Source & License Group -->
      <div class="section-title">Source &amp; License</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://github.com/itswill00/hypercore')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="github" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">GitHub Repository</div>
              <div class="row-sub">Source code &amp; issue tracker</div>
            </div>
          </div>
          <Icons name="chevron-right" :size="16" style="opacity: 0.4;" />
        </div>

        <div class="md3-list-row clickable" @click="openExternal('https://t.me/noticesa')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="about" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Telegram Channel</div>
              <div class="row-sub">Updates &amp; community support</div>
            </div>
          </div>
          <Icons name="chevron-right" :size="16" style="opacity: 0.4;" />
        </div>

        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="shield" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Open Source License</div>
              <div class="row-sub">GNU General Public License v3.0 (GPL-3.0)</div>
            </div>
          </div>
          <span class="badge-pill green">GPL v3</span>
        </div>
      </div>

      <div class="about-footer-text">
        HyperCore by @itswill00 • Free &amp; Open Source Software
      </div>

    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import { openExternal } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
</script>

<style scoped>
.about-app-banner {
  position: relative;
  border-radius: 18px;
  overflow: hidden;
  margin-bottom: 16px;
  border: 1px solid var(--surface-container-highest);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
  background-image: url('@/assets/banner.jpg');
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
  font-family: var(--font-mono);
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

.chip-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: var(--on-surface-variant);
}

.chip-dot.active {
  background: #34d399;
  box-shadow: 0 0 6px #34d399;
}

.about-footer-text {
  text-align: center;
  font-size: 10px;
  color: var(--on-surface-variant);
  opacity: 0.45;
  padding: 16px 0 24px 0;
  font-family: var(--font-mono);
}
</style>
