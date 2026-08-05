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
      
      <!-- Module Identity Banner -->
      <div class="about-app-banner">
        <div class="app-banner-icon">
          <Icons name="chip" :size="24" />
        </div>
        <div class="app-banner-meta">
          <div class="app-banner-title">HyperCore</div>
          <div class="app-banner-sub">Universal Kernel Optimizer for MediaTek Helio G99 Ultra</div>
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
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px;
  background: var(--surface-container);
  border: 1px solid var(--surface-container-high);
  border-radius: 16px;
  margin-bottom: 16px;
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
}

.app-banner-meta {
  min-width: 0;
}

.app-banner-title {
  font-size: 16px;
  font-weight: 700;
  color: var(--on-surface);
  letter-spacing: -0.2px;
}

.app-banner-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 2px;
  line-height: 1.4;
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
