<template>
  <div class="mb-6">
    <h3 class="section-title">QUICK ACTIONS</h3>
    <div class="md3-group">
      <div class="md3-list-item clickable" @click="flush">
        <div class="flex items-center gap-3.5">
          <div class="icon-badge">
            <Icons name="memory" :size="18" />
          </div>
          <div>
            <div class="item-title">Clear RAM Cache</div>
            <div class="item-sub">Compact memory & drop page caches</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" class="text-on-surface-variant" />
      </div>

      <div class="md3-list-item clickable" @click="saveLog">
        <div class="flex items-center gap-3.5">
          <div class="icon-badge secondary">
            <Icons name="logs" :size="18" />
          </div>
          <div>
            <div class="item-title">Save System Log</div>
            <div class="item-sub">Export log to /sdcard/HyperCore_log.txt</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" class="text-on-surface-variant" />
      </div>

      <div class="md3-list-item clickable" @click="restart">
        <div class="flex items-center gap-3.5">
          <div class="icon-badge tertiary">
            <Icons name="refresh" :size="18" />
          </div>
          <div>
            <div class="item-title">Restart Daemon</div>
            <div class="item-sub">Relaunch background C daemon</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" class="text-on-surface-variant" />
      </div>

      <div class="md3-list-item clickable" @click="addShortcut">
        <div class="flex items-center gap-3.5">
          <div class="icon-badge">
            <Icons name="star" :size="18" />
          </div>
          <div>
            <div class="item-title">Create App Shortcut</div>
            <div class="item-sub">Add WebUI launcher shortcut to home screen</div>
          </div>
        </div>
        <Icons name="chevron-right" :size="20" class="text-on-surface-variant" />
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
