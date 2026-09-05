<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">About Device</div>
        <div class="page-header-sub">System &amp; Community</div>
      </div>
    </div>

    <div class="content-area">

      <!-- About Hero Showcase Card with banner.jpg Background -->
      <div class="about-hero-banner" :style="{ backgroundImage: `url(${bannerImg})` }">
        <div class="about-banner-overlay"></div>
        <div class="about-banner-content">
          <div class="about-banner-top">
            <div class="about-platform-tag">
              <span>MT6789 Family</span>
            </div>
            <span class="about-version-badge">
              {{ store.moduleVersion || 'v6.4.8' }} · Stable
            </span>
          </div>

          <div class="about-title-block">
            <h1 class="about-hero-title">HyperCore</h1>
            <p class="about-hero-desc">
              Universal Dual-Kernel Performance Module
            </p>
            <div class="about-author-line">
              <span>Crafted with precision by</span>
              <a href="https://github.com/itswill00" target="_blank" class="about-author-link" @click.stop="openExternal('https://github.com/itswill00')">@itswill00</a>
            </div>
          </div>

          <div class="about-chips-row">
            <div class="about-glass-chip">
              <span class="chip-label">Kernel</span>
              <span class="chip-val">5.10 – 6.12 GKI</span>
            </div>
            <div class="about-glass-chip">
              <span class="chip-label">Target</span>
              <span class="chip-val">Helio G99/G100/G200</span>
            </div>
            <div class="about-glass-chip">
              <span class="chip-label">Arch</span>
              <span class="chip-val">ARM64-v8a</span>
            </div>
            <div class="about-glass-chip">
              <span class="chip-label">Daemon</span>
              <span class="chip-val" :style="store.isRunning ? 'color: var(--primary);' : ''">
                {{ store.isRunning ? `Active (PID ${store.daemonPid})` : 'Standby' }}
              </span>
            </div>
          </div>
        </div>
      </div>
      
      <!-- Release Notes (Expandable Changelog) -->
      <div class="md3-list-group">
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['changelog'] }"
          @click="toggleExpand('changelog')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge">
                <Icons name="logs" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">Release Notes</div>
                <div class="row-sub">Latest changes in {{ store.moduleVersion || 'v6.4.8' }}</div>
              </div>
            </div>
            <div class="row-val" style="margin-left: 8px;">
              <span class="changelog-badge">Changelog</span>
              <span class="expand-caret" :class="{ 'open': expandedRows['changelog'] }">▼</span>
            </div>
          </div>

          <div class="expanded-content">
            <div class="expanded-inner">
              <div class="changelog-release">
                <div class="release-header">
                  <span class="release-ver">{{ store.moduleVersion || 'v6.4.8' }} (Latest)</span>
                </div>
                <ul class="changelog-bullets">
                  <li><strong>Interactive Profile GPU Max Frequency Uncap</strong>: Removed artificial 648 MHz GPU ceiling in Interactive profile, opening full hardware capability (up to ~1003 MHz) with responsive 65% upthreshold to eliminate UI rendering stutter and frame drops.</li>
                  <li><strong>Real-Time Kernel Charging Telemetry</strong>: Replaced static text estimates with live kernel charging rate metrics directly on the Dashboard banner.</li>
                  <li><strong>WebUI Stylesheet Consolidation</strong>: Purged dead/unused CSS classes and consolidated root document styling rules for optimal rendering performance.</li>
                  <li><strong>Profile Transition State Hardening</strong>: Verified zero-friction, deterministic atomic transitions between Interactive, Gaming, MOBA, and Sleep states.</li>
                </ul>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Device Status Group -->
      <div class="section-title">Device Status</div>
      <div class="md3-list-group">
        
        <!-- Thermal Row -->
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['thermal'] }"
          @click="toggleExpand('thermal')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge">
                <Icons name="thermo" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">Temperature &amp; Thermal</div>
                <div class="row-sub">CPU &amp; battery thermal monitoring</div>
              </div>
            </div>
            <div class="row-val">
              <span class="expand-caret" :class="{ 'open': expandedRows['thermal'] }">▼</span>
            </div>
          </div>
          <div class="expanded-content">
            <div class="expanded-inner">
              <div class="stat-grid-2">
                <div class="stat-box">
                  <div class="stat-lbl">CPU temp</div>
                  <div class="stat-num">{{ store.cpuTemp }}°C</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Battery temp</div>
                  <div class="stat-num">{{ store.batTemp }}°C</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">GPU temp</div>
                  <div class="stat-num">{{ store.gpuTemp > 0 ? store.gpuTemp + '°C' : store.cpuTemp + '°C' }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Charger temp</div>
                  <div class="stat-num">{{ store.chgTemp > 0 ? store.chgTemp + '°C' : store.batTemp + '°C' }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Thermal tier</div>
                  <div class="stat-num">{{ store.thermalTier }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Charging status</div>
                  <div class="stat-num">{{ store.thermalGuardState }}</div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- Battery Row -->
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['battery'] }"
          @click="toggleExpand('battery')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge">
                <Icons name="battery" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">Battery &amp; Power</div>
                <div class="row-sub">Health, cycles &amp; power usage</div>
              </div>
            </div>
            <div class="row-val">
              <span class="expand-caret" :class="{ 'open': expandedRows['battery'] }">▼</span>
            </div>
          </div>
          <div class="expanded-content">
            <div class="expanded-inner">
              <div class="stat-grid-2">
                <div class="stat-box">
                  <div class="stat-lbl">Battery cycles</div>
                  <div class="stat-num">{{ store.batteryCycles > 0 ? store.batteryCycles + ' cycles' : '—' }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Health &amp; status</div>
                  <div class="stat-num">{{ store.batHealth }} ({{ store.batStatus }})</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Current flow</div>
                  <div class="stat-num">{{ store.batRate }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Voltage</div>
                  <div class="stat-num">{{ store.batVolt }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Full capacity</div>
                  <div class="stat-num">{{ store.batCapFull }}</div>
                </div>
                <div class="stat-box">
                  <div class="stat-lbl">Technology</div>
                  <div class="stat-num">{{ store.batTech }}</div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Hardware & System Group -->
      <div class="section-title">Hardware &amp; System</div>
      <div class="md3-list-group">
        
        <!-- Chipset Row -->
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['chipset'] }"
          @click="toggleExpand('chipset')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge">
                <Icons name="chip" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">Chipset Specs</div>
                <div class="row-sub">Processor &amp; GPU platform</div>
              </div>
            </div>
            <div class="row-val">
              <span class="expand-caret" :class="{ 'open': expandedRows['chipset'] }">▼</span>
            </div>
          </div>
          <div class="expanded-content">
            <div class="expanded-inner">
              Platform: MediaTek MT6789 (6nm)<br/>
              CPU: 2x Cortex-A76 @ 2.2 GHz &amp; 6x Cortex-A55 @ 2.0 GHz<br/>
              GPU: ARM Mali-G57 MC2 @ 950 MHz
            </div>
          </div>
        </div>

        <!-- Kernel Row -->
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['kernel'] }"
          @click="toggleExpand('kernel')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="icon-badge">
                <Icons name="cpu" :size="18" />
              </div>
              <div class="row-meta">
                <div class="row-title">Kernel Version</div>
                <div class="row-sub">Linux OS kernel details</div>
              </div>
            </div>
            <div class="row-val">
              <span class="expand-caret" :class="{ 'open': expandedRows['kernel'] }">▼</span>
            </div>
          </div>
          <div class="expanded-content">
            <div class="expanded-inner" style="white-space: normal; word-break: break-word;">
              Version: {{ store.kernelVersion && store.kernelVersion !== '—' ? store.kernelVersion : 'Linux' }}<br/>
              Optimizations: EAS Scheduler, Helio DVFSRC QoS Enabled
            </div>
          </div>
        </div>
      </div>

      <!-- System Specifications Group -->
      <div class="section-title">Module Specifications</div>
      <div class="md3-list-group">
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="chip" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Module Build</div>
              <div class="row-sub">Magisk / KernelSU / APatch Compatible</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill">Universal</span>
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
            <span class="badge-pill">0ms IPC</span>
          </div>
        </div>
      </div>

      <!-- Developer & Maintainer Group -->
      <div class="section-title">Developer &amp; Maintainer</div>
      <div class="md3-list-group">
        <div class="md3-list-row clickable" @click="openExternal('https://github.com/itswill00')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="pictogram-user" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@itswill00</div>
              <div class="row-sub">Lead Developer &amp; Architect</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary btn-md3-sm" @click.stop="openExternal('https://t.me/noticesa')">
            Telegram
          </button>
        </div>

        <div class="md3-list-row clickable" @click="openExternal('https://github.com/Rem01Gaming')">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="pictogram-user" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">@Rem01Gaming</div>
              <div class="row-sub">Inspiration &amp; Architecture Concepts (encore)</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-secondary btn-md3-sm" @click.stop="openExternal('https://github.com/Rem01Gaming')">
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
          <button class="btn-md3 btn-md3-secondary btn-md3-sm" @click.stop="openExternal('https://t.me/Rafzzz182')">
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
          <button class="btn-md3 btn-md3-secondary btn-md3-sm" @click.stop="openExternal('https://t.me/anotherside551')">
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
          <span class="badge-pill">GPL v3</span>
        </div>
      </div>

      <div class="about-footer-text">
        HyperCore by @itswill00 • Free &amp; Open Source Software
      </div>

    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { openExternal } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'
import bannerImg from '@/assets/banner.jpg'

const store = useHyperStore()
const expandedRows = ref({})

function toggleExpand(key) {
  expandedRows.value[key] = !expandedRows.value[key]
}
</script>

<style scoped>
.about-hero-banner {
  position: relative;
  border-radius: 18px;
  overflow: hidden;
  margin-bottom: 16px;
  border: 1px solid var(--surface-container-highest);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
  background-size: cover;
  background-position: center;
}

.about-banner-overlay {
  position: absolute;
  inset: 0;
  background: linear-gradient(135deg, rgba(14, 15, 20, 0.94) 0%, rgba(18, 19, 26, 0.84) 60%, rgba(12, 13, 18, 0.68) 100%);
  backdrop-filter: blur(2px);
  z-index: 1;
}

.about-banner-content {
  position: relative;
  z-index: 2;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.about-banner-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}

.about-platform-tag {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  font-size: 10.5px;
  font-weight: 600;
  color: var(--primary);
  background: rgba(255, 255, 255, 0.08);
  padding: 3px 9px;
  border-radius: 20px;
  border: 1px solid rgba(255, 255, 255, 0.12);
  letter-spacing: 0.2px;
  backdrop-filter: blur(4px);
}

.about-version-badge {
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  font-size: 10px;
  font-weight: 600;
  padding: 3px 8px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.08);
  color: var(--on-surface);
  border: 1px solid rgba(255, 255, 255, 0.12);
  backdrop-filter: blur(4px);
}

.about-title-block {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.about-hero-title {
  margin: 0;
  font-size: 22px;
  font-weight: 800;
  color: var(--on-surface);
  letter-spacing: -0.4px;
  line-height: 1.15;
}

.about-hero-desc {
  margin: 0;
  font-size: 12px;
  color: var(--on-surface-variant);
  line-height: 1.4;
  opacity: 0.9;
}

.about-author-line {
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 11px;
  color: var(--on-surface-variant);
  opacity: 0.75;
  margin-top: 2px;
}

.about-author-link {
  color: var(--primary);
  text-decoration: none;
  font-weight: 600;
}

.about-chips-row {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  padding-top: 8px;
  border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.about-glass-chip {
  font-family: inherit;
  font-variant-numeric: tabular-nums;
  font-size: 10px;
  padding: 3px 8px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.12);
  display: flex;
  align-items: center;
  gap: 5px;
  backdrop-filter: blur(4px);
}

.about-glass-chip .chip-label {
  color: var(--on-surface-variant);
  opacity: 0.7;
}

.about-glass-chip .chip-val {
  color: var(--on-surface);
  font-weight: 600;
}

.app-banner-icon {
  width: 36px;
  height: 36px;
  border-radius: 12px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.app-banner-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface);
  letter-spacing: -0.2px;
}

.app-banner-sub {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-top: 1px;
  line-height: 1.35;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.changelog-badge {
  font-size: 10.5px;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 6px;
  background: var(--surface-container-highest);
  color: var(--on-surface-variant);
  letter-spacing: 0.2px;
}

.changelog-timeline {
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding-left: 2px;
}

.changelog-release {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.release-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.release-ver {
  font-size: 12px;
  font-weight: 700;
  color: var(--on-surface);
  font-family: inherit;
  font-variant-numeric: tabular-nums;
}

.release-date {
  font-size: 10px;
  color: var(--on-surface-variant);
  opacity: 0.6;
}

.changelog-bullets {
  margin: 0;
  padding-left: 16px;
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.5;
}

.changelog-bullets li {
  margin-bottom: 2px;
}

.about-footer-text {
  text-align: center;
  font-size: 10px;
  color: var(--on-surface-variant);
  opacity: 0.45;
  padding: 16px 0 24px 0;
  font-family: inherit;
}
</style>
