<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">About HyperCore</div>
        <div class="page-header-sub">Module information, contributors &amp; license</div>
      </div>
      <span class="badge-pill purple" style="font-size: 11px; padding: 4px 10px;">
        {{ store.moduleVersion || 'v4.3' }}
      </span>
    </div>

    <div class="content-area">
      
      <!-- Hero Card -->
      <div class="md3-card about-hero">
        <div class="hero-header">
          <div class="icon-badge primary" style="width: 44px; height: 44px;">
            <Icons name="chip" :size="22" />
          </div>
          <div>
            <div class="hero-title">HyperCore</div>
            <div class="hero-subtitle">Kernel Optimizer &amp; Hardware Daemon</div>
          </div>
        </div>
        <div class="hero-body">
          Lightweight zero-overhead background daemon tailored for MediaTek Helio G99 Ultra (MT6789) devices. Automates CPU governors, GPU devfreq scaling, DVFSRC interconnect bandwidth, and storage read-ahead queues.
        </div>
        <div class="hero-tags">
          <span class="tag-chip">SoC: MT6789 (Helio G99)</span>
          <span class="tag-chip">Kernel: 5.10.x</span>
          <span class="tag-chip">Arch: ARM64</span>
          <span class="tag-chip">License: GPL v3</span>
        </div>
      </div>

      <!-- Developer & Inspiration -->
      <div class="section-title">Developer &amp; Inspiration</div>
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

      <!-- Testing Team -->
      <div class="section-title">Testing Team</div>
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

      <!-- License & Repository -->
      <div class="section-title">License &amp; Source Code</div>
      <div class="md3-card">
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px;">
          <div style="font-size: 13px; font-weight: 600; color: var(--on-surface);">GNU General Public License v3.0</div>
          <span class="badge-pill green" style="font-size: 10px; padding: 2px 8px;">GPL v3.0</span>
        </div>
        <div style="font-size: 11px; color: var(--on-surface-variant); line-height: 1.5; margin-bottom: 12px;">
          HyperCore is Free Open-Source Software. You are free to redistribute and modify it under the terms of the GNU General Public License as published by the Free Software Foundation.
        </div>
        <div style="display: flex; gap: 8px;">
          <button class="btn-md3 btn-md3-primary" style="flex: 1; font-size: 11px;" @click="openExternal('https://github.com/itswill00/hypercore')">
            <Icons name="github" :size="14" />
            <span>GitHub Repository</span>
          </button>
          <button class="btn-md3 btn-md3-secondary" style="flex: 1; font-size: 11px;" @click="openExternal('https://t.me/noticesa')">
            <Icons name="about" :size="14" />
            <span>Telegram Channel</span>
          </button>
        </div>
      </div>

      <div style="text-align: center; font-size: 10px; color: var(--on-surface-variant); opacity: 0.5; padding: 12px 0 24px 0; font-family: var(--font-mono);">
        HyperCore by @itswill00 • Inspired by encore @Rem01Gaming
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
.about-hero {
  background: var(--surface-container-high);
  border: 1px solid var(--surface-bright);
}

.hero-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 10px;
}

.hero-title {
  font-size: 16px;
  font-weight: 700;
  color: var(--on-surface);
}

.hero-subtitle {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.hero-body {
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.5;
  margin-bottom: 12px;
}

.hero-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.tag-chip {
  font-family: var(--font-mono);
  font-size: 9.5px;
  padding: 3px 8px;
  border-radius: 6px;
  background: var(--surface-container-low);
  color: var(--on-surface-variant);
  border: 1px solid var(--outline-variant);
}

.avatar-badge {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 700;
  font-size: 12px;
  flex-shrink: 0;
}

.avatar-badge.dev {
  background: rgba(168, 85, 247, 0.2);
  color: #c084fc;
}

.avatar-badge.inspiration {
  background: rgba(59, 130, 246, 0.2);
  color: #60a5fa;
}

.avatar-badge.tester {
  background: rgba(16, 185, 129, 0.2);
  color: #34d399;
}
</style>
