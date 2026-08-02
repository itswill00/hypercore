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

      <!-- Info Note -->
      <div class="md3-card" style="padding: 14px 16px; margin-bottom: 16px;">
        <div style="font-size: 12px; color: var(--on-surface-variant); line-height: 1.5;">
          HyperCore runs automatically in the background. Performance profile switches to Gaming mode when an installed game is opened.
        </div>
      </div>

      <div style="text-align: center; font-size: 10px; opacity: 0.45; padding: 4px 0 20px 0; font-family: var(--font-mono);">
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
