<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">About Device</div>
        <div class="page-header-sub">Module Info &amp; Community</div>
      </div>
      <span class="badge-pill" style="font-size: 11px; padding: 4px 10px;">
        {{ store.moduleVersion || 'v6.3.5' }}
      </span>
    </div>

    <div class="content-area">

      <!-- Module Info Card -->
      <div class="md3-card mb-3">
        <div class="about-hero">
          <div class="hero-icon">
            <Icons name="zap" :size="32" />
          </div>
          <div>
            <div class="hero-title">HyperCore</div>
            <div class="hero-sub">Universal Dual-Kernel MT6789 Performance Module</div>
          </div>
        </div>

        <div style="border-top: 1px solid var(--outline-variant); margin: 0 -16px;">
          <div class="md3-list-row">
            <div class="row-left">
              <div class="row-title">Installed Version</div>
            </div>
            <div class="row-val">
              <span class="release-ver">{{ store.moduleVersion || 'v6.3.5' }} (Latest)</span>
            </div>
          </div>
        </div>
      </div>
      
      <!-- Module Identity Banner (Expandable Latest Changelog) -->
      <div class="md3-list-group">
        <div
          class="md3-list-row expandable-row clickable"
          :class="{ 'is-expanded': expandedRows['changelog'] }"
          @click="toggleExpand('changelog')"
        >
          <div class="row-header">
            <div class="row-left">
              <div class="app-banner-icon">
                <Icons name="chip" :size="22" />
              </div>
              <div class="row-meta">
                <div class="app-banner-title">HyperCore Module</div>
                <div class="app-banner-sub">Build Specs &amp; Release Changelog</div>
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
                  <span class="release-ver">{{ store.moduleVersion || 'v6.3.5' }} (Latest)</span>
                </div>
                <ul class="changelog-bullets">
                  <li><strong>100% Hands-off OEM Stock Architecture</strong>: Complete hands-off decoupling in OEM Stock mode with zero periodic sysfs writes, restoring 100% thermal throttling control to Xiaomi HyperOS kernel and native mi_thermald.</li>
                  <li><strong>Physical Hardware Charging Calibration</strong>: Empirically re-calibrated Balanced mode (Limit 10, ~2.3A / 9.3W) and Safe mode (Limit 14, ~0.73A / 2.8W) with automatic 80% SOC longevity tapering.</li>
                  <li><strong>Single Source of Truth Battery Cycles</strong>: Eliminated conflicting WebUI background polling and software cycle accumulation; strictly reads physical MediaTek BMS Fuel Gauge Coulomb counters directly.</li>
                  <li><strong>Exclusive Internal Storage Builds</strong>: Standardized unified release naming without duplicate git hash artifacts, strictly outputting to device internal storage (/sdcard/HyperCore_Releases).</li>
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
                  <div class="stat-lbl">Power guard</div>
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

const store = useHyperStore()
const expandedRows = ref({})

function toggleExpand(key) {
  expandedRows.value[key] = !expandedRows.value[key]
}
</script>

<style scoped>
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
  font-family: var(--font-mono);
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
  font-family: var(--font-mono);
}
</style>
