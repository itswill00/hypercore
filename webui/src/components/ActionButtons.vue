<template>
  <div class="btn-row">
    <button class="btn" :disabled="store.loading" @click="flush">Clear RAM</button>
    <button class="btn" :disabled="store.loading" @click="restart">Restart daemon</button>
  </div>
  <div class="btn-row" style="margin-bottom: 16px;">
    <button class="btn" :disabled="store.loading" @click="saveLog">Save log</button>
    <button class="btn" @click="addShortcut">Create shortcut</button>
  </div>
</template>

<script setup>
import { inject } from 'vue'
import { useHyperStore } from '@/stores/hyper'

const store = useHyperStore()
const toast = inject('toast')

async function flush() {
  const msg = await store.flushRam()
  toast(msg)
}

async function restart() {
  const msg = await store.restartDaemon()
  toast(msg)
}

async function saveLog() {
  const msg = await store.exportLogs()
  toast(msg)
}

function addShortcut() {
  const msg = store.createShortcut()
  toast(msg)
}
</script>

<style scoped>
.btn-row { display: flex; gap: 8px; margin-bottom: 8px; }
.btn-row .btn { flex: 1; }
</style>
