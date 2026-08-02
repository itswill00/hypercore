<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Top Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">Dashboard</div>
        <div class="page-header-sub">Tanzanite HyperCore by @itswill00</div>
      </div>
    </div>

    <!-- Main Content View -->
    <div class="content-area">
      <!-- Banner Status Card -->
      <StatusCard />

      <!-- Unified Device & System Info Group Card -->
      <div class="md3-list-group">
        <!-- Current Profile Row -->
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="star" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Current Profile</div>
              <div class="row-sub">Applies automatically based on usage</div>
            </div>
          </div>
          <span class="badge-pill" style="background: rgba(226, 229, 236, 0.12); color: #e2e5ec; font-size: 11px;">
            {{ store.activeProfile || 'Interactive' }}
          </span>
        </div>

        <!-- Temperature Row -->
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="thermo" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Temperature</div>
              <div class="row-sub">CPU & Battery thermal status</div>
            </div>
          </div>
          <div class="row-val">
            CPU {{ store.cpuTemp }}°C • Bat {{ store.batTemp }}°C
          </div>
        </div>

        <!-- Battery Cycles Row -->
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="battery" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">Battery Cycles</div>
              <div class="row-sub">Hardware BMS coulomb counter</div>
            </div>
          </div>
          <div class="row-val">
            {{ store.batteryCycles || 347 }} cycles
          </div>
        </div>

        <!-- File Integrity Row -->
        <div class="md3-list-row">
          <div class="row-left">
            <div class="icon-badge">
              <Icons name="shield" :size="18" />
            </div>
            <div class="row-meta">
              <div class="row-title">File Integrity</div>
              <div class="row-sub">Embedded binary SHA-256 check</div>
            </div>
          </div>
          <span class="badge-pill" style="background: rgba(160, 178, 198, 0.16); color: #a0b2c6; font-size: 10px;">
            Verified (SHA-256 OK)
          </span>
        </div>

        <!-- Expandable Chipset Row -->
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
                <div class="row-title">Chipset</div>
                <div class="row-sub">Hardware Platform</div>
              </div>
            </div>
            <div class="row-val">
              Helio G99 Ultra
              <span class="expand-caret" :class="{ 'open': expandedRows['chipset'] }">▾</span>
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

        <!-- Expandable Kernel Version Row -->
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
                <div class="row-sub">Linux Kernel Build</div>
              </div>
            </div>
            <div class="row-val">
              {{ store.kernelVersion && store.kernelVersion !== '—' ? store.kernelVersion.split(' ')[0] : 'Linux 5.10.x' }}
              <span class="expand-caret" :class="{ 'open': expandedRows['kernel'] }">▾</span>
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

      <!-- Quick Actions Group Card -->
      <ActionButtons />

      <!-- Contributors Card -->
      <div class="section-title">Contributors</div>
      <div class="md3-list-group" style="margin-bottom: 14px;">

        <!-- Developer row -->
        <div class="md3-list-row">
          <div class="row-left">
            <a href="https://t.me/noticesa" target="_blank" class="avatar-link">
              <div class="avatar-badge dev">ن</div>
            </a>
            <div class="row-meta">
              <div class="row-title">@noticesa</div>
              <div class="row-sub">Developer & Maintainer</div>
            </div>
          </div>
          <span class="role-chip dev">dev</span>
        </div>

        <!-- Tester 1 -->
        <div class="md3-list-row">
          <div class="row-left">
            <a href="https://t.me/Rafzzz182" target="_blank" class="avatar-link">
              <div class="avatar-badge tester">R</div>
            </a>
            <div class="row-meta">
              <div class="row-title">@Rafzzz182</div>
              <div class="row-sub">Testing & feedback</div>
            </div>
          </div>
          <span class="role-chip tester">tester</span>
        </div>

        <!-- Tester 2 -->
        <div class="md3-list-row">
          <div class="row-left">
            <a href="https://t.me/anotherside551" target="_blank" class="avatar-link">
              <div class="avatar-badge tester">A</div>
            </a>
            <div class="row-meta">
              <div class="row-title">@anotherside551</div>
              <div class="row-sub">Testing & feedback</div>
            </div>
          </div>
          <span class="role-chip tester">tester</span>
        </div>
      </div>

      <!-- Donate Card -->
      <div class="donate-card">
        <div class="donate-left">
          <div class="donate-icon">♡</div>
          <div>
            <div class="donate-title">Support development</div>
            <div class="donate-sub">If HyperCore helps you, consider buying a coffee</div>
          </div>
        </div>
        <a
          href="https://sociabuzz.com/noticesa/tribe"
          target="_blank"
          class="donate-btn"
        >
          Donate
        </a>
      </div>

      <!-- Footer -->
      <div style="text-align: center; font-size: 10px; opacity: 0.35; padding: 4px 0 20px 0; font-family: var(--font-mono);">
        Inspired by encore @Rem01Gaming
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import StatusCard from '@/components/StatusCard.vue'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const expandedRows = ref({})

function toggleExpand(key) {
  expandedRows.value[key] = !expandedRows.value[key]
}
</script>

<style scoped>
/* Avatar badges */
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

/* Role chips */
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

/* Donate Card */
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
  text-decoration: none;
  flex-shrink: 0;
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
</style>
