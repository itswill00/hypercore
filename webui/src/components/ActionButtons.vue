<template>
  <div>
    <div class="section-title">Quick Actions</div>
    <div class="md3-list-group">
      <div class="md3-list-row clickable" @click="flush">
        <div class="row-left">
          <div class="icon-badge">
            <Icons name="memory" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Clear RAM Cache</div>
            <div class="row-sub">Compact memory & drop page caches</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" @click="saveLog">
        <div class="row-left">
          <div class="icon-badge secondary">
            <Icons name="logs" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Save System Log</div>
            <div class="row-sub">Export log to /sdcard/HyperCore_log.txt</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" @click="restart">
        <div class="row-left">
          <div class="icon-badge tertiary">
            <Icons name="refresh" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Restart Daemon</div>
            <div class="row-sub">Relaunch background C daemon</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>

      <div class="md3-list-row clickable" @click="addShortcut">
        <div class="row-left">
          <div class="icon-badge">
            <Icons name="star" :size="18" />
          </div>
          <div class="row-meta">
            <div class="row-title">Create App Shortcut</div>
            <div class="row-sub">Add WebUI shortcut to home screen</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" style="color: var(--on-surface-variant);" />
      </div>
    </div>
  </div>
</template>

<script setup>
import { inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
const toast = inject('toast')

async function flush() {
  const msg = await store.flushRam()
  if (toast) toast(msg)
}

async function restart() {
  const msg = await store.restartDaemon()
  if (toast) toast(msg)
}

async function saveLog() {
  const msg = await store.exportLogs()
  if (toast) toast(msg)
}

function addShortcut() {
  const msg = store.createShortcut()
  if (toast) toast(msg)
}
</script>
