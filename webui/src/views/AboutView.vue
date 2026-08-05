<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <!-- Page Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">About Device</div>
        <div class="page-header-sub">Hardware status, specifications &amp; credits</div>
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
          <div class="app-banner-sub">Universal Kernel Optimizer for MediaTek MT6789 Family</div>
        </div>
      </div>

      <!-- Device Status Group -->
      <div class="section-title">Device Status</div>
      <div class="md3-list-group" style="margin-bottom: 14px;">
        
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
                  <div class="stat-num" :class="store.tempColor">{{ store.cpuTemp }}°C</div>
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
                  <div class="stat-num" :class="store.thermalColor">{{ store.thermalTier }}</div>
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
      <div class="md3-list-group" style="margin-bottom: 14px;">
        
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
              Version: {{ store.kernelVersion && store.kernelVersion !== '—' ? store.kernelVersion : 'Linux 5.10.x' }}<br/>
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
              <div class="row-title">Target Platform</div>
              <div class="row-sub">MediaTek MT6789 Family</div>
            </div>
          </div>
          <div class="row-val">
            <span class="badge-pill purple">MT6789</span>
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

        <div class="md3-list-row clickable" @click="handleCreateShortcut">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="about" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Launcher Shortcut</div>
              <div class="row-sub">Pin WebUI shortcut to Android home screen</div>
            </div>
          </div>
          <button class="btn-md3 btn-md3-primary" style="padding: 4px 10px; font-size: 10px;" @click.stop="handleCreateShortcut">
            Add Shortcut
          </button>
        </div>
      </div>

      <!-- Developer & Maintainer Group -->
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
import { ref } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { openExternal, createShortcut } from '@/helpers/shell'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const expandedRows = ref({})

function toggleExpand(key) {
  expandedRows.value[key] = !expandedRows.value[key]
}

function handleCreateShortcut() {
  const ok = createShortcut()
  if (!ok) {
    alert('To add a launcher shortcut, long-press HyperCore in KernelSU / APatch Manager or tap 3-dots -> "Add to Home Screen".')
  }
}
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
