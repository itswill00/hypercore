<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">About Device &amp; Module</div>
        <div class="page-header-sub">HyperCore specifications, credits &amp; license</div>
      </div>
      <span class="badge-pill purple" style="font-size: 11px; padding: 4px 10px;">
        {{ store.moduleVersion || 'v4.3' }}
      </span>
    </div>

    <div class="content-area">
      
      <!-- OxygenOS Style Hero Device Card -->
      <div class="oxygen-hero-card">
        <div class="oxygen-hero-header">
          <div>
            <div class="oxygen-hero-title">HyperCore</div>
            <div class="oxygen-hero-sub">Helio G99 Hardware Tuning Engine</div>
          </div>
          <div class="oxygen-ver-badge">
            {{ store.moduleVersion || 'v4.3' }}
          </div>
        </div>

        <div class="oxygen-hero-desc">
          Universal background daemon and kernel tuning module for MediaTek MT6789 (Helio G99 Ultra) devices running Linux Kernel 5.10.x.
        </div>

        <!-- 2x2 Specs Grid -->
        <div class="oxygen-spec-grid">
          <div class="oxygen-spec-tile">
            <div class="spec-tile-icon">
              <Icons name="chip" :size="16" />
            </div>
            <div class="spec-tile-meta">
              <div class="spec-tile-label">Processor</div>
              <div class="spec-tile-val">Helio G99 (MT6789)</div>
            </div>
          </div>

          <div class="oxygen-spec-tile">
            <div class="spec-tile-icon">
              <Icons name="cpu" :size="16" />
            </div>
            <div class="spec-tile-meta">
              <div class="spec-tile-label">Kernel</div>
              <div class="spec-tile-val">{{ store.kernelVersion ? store.kernelVersion.split('-')[0] : 'Linux 5.10.x' }}</div>
            </div>
          </div>

          <div class="oxygen-spec-tile">
            <div class="spec-tile-icon">
              <Icons name="shield" :size="16" />
            </div>
            <div class="spec-tile-meta">
              <div class="spec-tile-label">Architecture</div>
              <div class="spec-tile-val">ARM64 (aarch64)</div>
            </div>
          </div>

          <div class="oxygen-spec-tile">
            <div class="spec-tile-icon">
              <Icons name="zap" :size="16" />
            </div>
            <div class="spec-tile-meta">
              <div class="spec-tile-label">IPC Daemon</div>
              <div class="spec-tile-val">C Native (0ms IPC)</div>
            </div>
          </div>
        </div>
      </div>

      <!-- Developer & Maintainer Section -->
      <div class="section-title">Developer &amp; Maintainer</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://github.com/itswill00')">
          <div class="row-left">
            <div class="avatar-badge dev">ن</div>
            <div class="row-meta">
              <div class="row-title">@itswill00</div>
              <div class="row-sub">Lead Developer &amp; Architect (Telegram: @noticesa)</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary" style="padding: 4px 10px; font-size: 10px;" @click.stop="openExternal('https://t.me/noticesa')">
            Telegram
          </button>
        </div>

        <div class="md3-list-row clickable" @click="openExternal('https://github.com/Rem01Gaming')">
          <div class="row-left">
            <div class="avatar-badge inspiration">R</div>
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

      <!-- Testing Team Section -->
      <div class="section-title">Quality Assurance &amp; Testing</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://t.me/Rafzzz182')">
          <div class="row-left">
            <div class="avatar-badge tester">R</div>
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
            <div class="avatar-badge tester">A</div>
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

      <!-- License & Open Source Section -->
      <div class="section-title">License &amp; Source Code</div>
      <div class="md3-card">
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px;">
          <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">GNU General Public License v3.0</div>
          <span class="badge-pill green" style="font-size: 10px; padding: 2px 8px;">GPL v3.0</span>
        </div>
        <div style="font-size: 11px; color: var(--on-surface-variant); line-height: 1.5; margin-bottom: 14px;">
          HyperCore is Free Open-Source Software. You are free to redistribute and modify it under the terms of the GNU General Public License v3.0 as published by the Free Software Foundation.
        </div>
        
        <div class="oxygen-action-grid">
          <button class="btn-md3 btn-md3-primary" style="font-size: 11px;" @click="openExternal('https://github.com/itswill00/hypercore')">
            <Icons name="github" :size="14" />
            <span>GitHub Repository</span>
          </button>
          <button class="btn-md3 btn-md3-secondary" style="font-size: 11px;" @click="openExternal('https://t.me/noticesa')">
            <Icons name="about" :size="14" />
            <span>Telegram Channel</span>
          </button>
        </div>
      </div>

      <div class="oxygen-footer">
        HyperCore for MediaTek Helio G99 • Free &amp; Open Source Software
      </div>

    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()

function openExternal(url) {
  if (typeof ksu !== 'undefined' && typeof ksu.open === 'function') {
    ksu.open(url)
  } else {
    window.open(url, '_blank')
  }
}
</script>

<style scoped>
.oxygen-hero-card {
  background: linear-gradient(135deg, var(--surface-container-high) 0%, var(--surface-container) 100%);
  border-radius: 20px;
  padding: 16px;
  margin-bottom: 14px;
  border: 1px solid var(--surface-container-highest);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.25);
}

.oxygen-hero-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 10px;
}

.oxygen-hero-title {
  font-size: 20px;
  font-weight: 800;
  color: var(--on-surface);
  letter-spacing: -0.3px;
}

.oxygen-hero-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 1px;
}

.oxygen-ver-badge {
  background: var(--primary-container);
  color: var(--on-primary-container);
  font-family: var(--font-mono);
  font-size: 11px;
  font-weight: 700;
  padding: 4px 10px;
  border-radius: 12px;
  border: 1px solid var(--outline-variant);
}

.oxygen-hero-desc {
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.5;
  margin-bottom: 14px;
}

.oxygen-spec-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 8px;
}

.oxygen-spec-tile {
  background: var(--surface-container-low);
  border: 1px solid var(--outline-variant);
  border-radius: 12px;
  padding: 10px 12px;
  display: flex;
  align-items: center;
  gap: 10px;
}

.spec-tile-icon {
  width: 28px;
  height: 28px;
  border-radius: 8px;
  background: var(--surface-container-high);
  color: var(--primary);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.spec-tile-meta {
  min-width: 0;
}

.spec-tile-label {
  font-size: 9.5px;
  color: var(--on-surface-variant);
  opacity: 0.8;
}

.spec-tile-val {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface);
  font-family: var(--font-mono);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.avatar-badge {
  width: 34px;
  height: 34px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 700;
  font-size: 12px;
  flex-shrink: 0;
}

.avatar-badge.dev {
  background: var(--primary-container);
  color: var(--on-primary-container);
}

.avatar-badge.inspiration {
  background: var(--secondary-container);
  color: var(--on-secondary-container);
}

.avatar-badge.tester {
  background: var(--tertiary-container);
  color: var(--on-tertiary-container);
}

.oxygen-action-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 8px;
}

.oxygen-footer {
  text-align: center;
  font-size: 10px;
  color: var(--on-surface-variant);
  opacity: 0.5;
  padding: 16px 0 24px 0;
  font-family: var(--font-mono);
}
</style>
