<template>
  <div class="btn-row">
    <button class="btn" :disabled="store.loading" @click="flush">Clear RAM cache</button>
    <button class="btn" :disabled="store.loading" @click="restart">Restart daemon</button>
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
</script>

<style scoped>
.btn-row { display: flex; gap: 8px; margin-bottom: 12px; }
.btn-row .btn { flex: 1; }
</style>
