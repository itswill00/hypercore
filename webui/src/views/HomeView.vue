<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    
    <div class="page-header">
      <div>
        <div class="page-header-title">HyperCore</div>
        <div class="page-header-sub">by aerichandesu@noticesa</div>
      </div>
    </div>

    <div class="content-area">
      
      <!-- Home Hero Banner Card with banner.jpg Background -->
      <div class="home-app-banner">
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
              <span>Daemon: {{ store.isRunning ? `Active (PID ${store.daemonPid})` : 'Standby' }}</span>
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

      <StatusCard />

      <div class="section-title">Device Status</div>
      <div class="md3-list-group" style="margin-bottom: 14px;">
        
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

      <div class="section-title">Hardware &amp; System</div>
      <div class="md3-list-group" style="margin-bottom: 14px;">
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

      <ActionButtons />

      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 12px 0 20px 0; font-family: var(--font-mono);">
        Inspired by encore @Rem01Gaming
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import { execCommand } from '@/helpers/shell'
import StatusCard from '@/components/StatusCard.vue'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const expandedRows = ref({})
const showContributorsModal = ref(false)

function toggleExpand(key) {
  expandedRows.value[key] = !expandedRows.value[key]
}

async function openExternal(url) {
  try {
    if (typeof ksu !== 'undefined' && typeof ksu.open === 'function') {
      ksu.open(url)
      return
    }
  } catch (e) {}

  try {
    await execCommand(`am start -a android.intent.action.VIEW -d "${url}" 2>/dev/null`)
    return
  } catch (e) {}

  const a = document.createElement('a')
  a.href = url
  a.target = '_blank'
  a.rel = 'noopener noreferrer'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
}
</script>

<style scoped>

.avatar-link {
  text-decoration: none;
  flex-shrink: 0;
}

.avatar-badge {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 14px;
  font-weight: 700;
  flex-shrink: 0;
  transition: transform 0.15s cubic-bezier(0.34, 1.56, 0.64, 1);
}

.avatar-badge:active {
  transform: scale(0.88);
}

.avatar-badge.dev {
  background: rgba(113, 107, 168, 0.22);
  color: #b8b3e8;
  border: 1px solid rgba(113, 107, 168, 0.3);
}

.avatar-badge.tester {
  background: rgba(160, 178, 198, 0.14);
  color: #a0b2c6;
  border: 1px solid rgba(160, 178, 198, 0.2);
}

.role-chip {
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.6px;
  padding: 2px 8px;
  border-radius: 20px;
  text-transform: uppercase;
  flex-shrink: 0;
}

.role-chip.dev {
  background: rgba(113, 107, 168, 0.18);
  color: #b8b3e8;
}

.role-chip.tester {
  background: rgba(160, 178, 198, 0.12);
  color: #8fa0b0;
}

.donate-card {
  background: linear-gradient(135deg,
    rgba(113, 107, 168, 0.14) 0%,
    rgba(160, 178, 198, 0.08) 100%
  );
  border: 1px solid rgba(113, 107, 168, 0.22);
  border-radius: 16px;
  padding: 14px 14px 14px 16px;
  margin-bottom: 14px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  animation: banner-enter 0.4s cubic-bezier(0.34, 1.56, 0.64, 1) both;
}

.donate-left {
  display: flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
}

.donate-icon {
  font-size: 20px;
  flex-shrink: 0;
  line-height: 1;
  filter: drop-shadow(0 0 6px rgba(180, 160, 220, 0.4));
}

.donate-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface);
}

.donate-sub {
  font-size: 10px;
  color: var(--on-surface-variant);
  margin-top: 1px;
  opacity: 0.8;
  line-height: 1.4;
}

.donate-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: rgba(113, 107, 168, 0.28);
  color: #c4bef5;
  border: 1px solid rgba(113, 107, 168, 0.35);
  border-radius: 10px;
  padding: 7px 14px;
  font-size: 12px;
  font-weight: 600;
  font-family: inherit;
  flex-shrink: 0;
  cursor: pointer;
  -webkit-appearance: none;
  appearance: none;
  transition: transform 0.12s cubic-bezier(0.34, 1.56, 0.64, 1),
              background 0.15s ease,
              filter 0.15s ease;
  user-select: none;
}

.donate-btn:hover {
  background: rgba(113, 107, 168, 0.4);
  filter: brightness(1.08);
}

.donate-btn:active {
  transform: scale(0.93);
  transition: transform 0.08s ease;
}

.preview-stack {
  display: flex;
  align-items: center;
  gap: 0;
}

.preview-avatar {
  width: 22px;
  height: 22px;
  border-radius: 50%;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 10px;
  font-weight: 700;
  margin-left: -6px;
  border: 2px solid var(--surface, #12141a);
}

.preview-avatar.dev {
  background: var(--surface-container-highest, #30323a);
  color: var(--on-surface, #f0f2f5);
  z-index: 3;
}

.preview-avatar.tester {
  background: var(--surface-container-high, #26272e);
  color: var(--on-surface-variant, #b0b4c0);
  z-index: 2;
}

/* Modal Window Overlay */
.modal-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(10, 11, 13, 0.78);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  z-index: 999;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  padding: 0;
}

@media (min-width: 480px) {
  .modal-backdrop {
    align-items: center;
    padding: 20px;
  }
}

.modal-window {
  width: 100%;
  max-width: 440px;
  background: var(--surface-container-high, #26272e);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 24px 24px 0 0;
  padding: 20px;
  box-shadow: 0 -12px 40px rgba(0, 0, 0, 0.55);
  max-height: 85vh;
  overflow-y: auto;
}

@media (min-width: 480px) {
  .modal-window {
    border-radius: 24px;
  }
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--outline-variant, #383a42);
}

.modal-title {
  font-size: 16px;
  font-weight: 700;
  color: var(--on-surface, #f0f2f5);
}

.modal-sub {
  font-size: 11px;
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.8;
}

.modal-close {
  width: 30px;
  height: 30px;
  border-radius: 50%;
  border: 1px solid var(--outline-variant, #383a42);
  background: var(--surface-container, #1d1e23);
  color: var(--on-surface, #f0f2f5);
  font-size: 13px;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: transform 0.12s ease;
}

.modal-close:active {
  transform: scale(0.88);
}

.modal-section-title {
  font-size: 11px;
  font-weight: 600;
  color: var(--blue, #a0b2c6);
  opacity: 0.9;
  margin-bottom: 8px;
}

.contributor-card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: var(--surface-container, #1d1e23);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 14px;
  padding: 10px 12px;
}

.contributor-left {
  display: flex;
  align-items: center;
  gap: 10px;
}

.contributor-info {
  display: flex;
  flex-direction: column;
}

.contributor-name {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface, #f0f2f5);
}

.contributor-role {
  font-size: 10px;
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.8;
}

.action-chip {
  background: var(--primary-container, #33353e);
  color: var(--on-primary-container, #f0f2f5);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 20px;
  padding: 5px 12px;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.12s ease, background 0.15s ease;
}

.action-chip:active {
  transform: scale(0.92);
}

.donate-banner {
  background: var(--surface-container, #1d1e23);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 14px;
  padding: 12px 14px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}

/* Silky-Smooth GPU-Accelerated Vue Modal Transition */
.modal-enter-active {
  transition: opacity 0.3s cubic-bezier(0.32, 0.72, 0, 1);
}

.modal-leave-active {
  transition: opacity 0.22s cubic-bezier(0.32, 0.72, 0, 1);
}

.modal-enter-active .modal-window {
  transition: transform 0.35s cubic-bezier(0.32, 0.72, 0, 1), opacity 0.35s cubic-bezier(0.32, 0.72, 0, 1);
}

.modal-leave-active .modal-window {
  transition: transform 0.22s cubic-bezier(0.32, 0.72, 0, 1), opacity 0.22s cubic-bezier(0.32, 0.72, 0, 1);
}

.modal-enter-from {
  opacity: 0;
}

.modal-enter-from .modal-window {
  opacity: 0;
  transform: translate3d(0, 50px, 0) scale(0.96);
}

.modal-leave-to {
  opacity: 0;
}

.modal-leave-to .modal-window {
  opacity: 0;
  transform: translate3d(0, 30px, 0) scale(0.97);
}

.home-app-banner {
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
</style>
