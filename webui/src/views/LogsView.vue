<template>
  <div class="card">
    <div class="card-title">
      <span>System log</span>
      <span class="badge clickable" @click="store.refresh()">Refresh</span>
    </div>
    <pre class="log-box">{{ store.logs }}</pre>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import { onMounted, nextTick, watch } from 'vue'

const store = useHyperStore()

watch(() => store.logs, async () => {
  await nextTick()
  const el = document.querySelector('.log-box')
  if (el) el.scrollTop = el.scrollHeight
})
</script>

<style scoped>
.log-box {
  background: var(--surface-alt);
  border: 1px solid var(--border);
  padding: 8px 10px;
  border-radius: var(--radius-sm);
  font-size: 10px;
  font-family: var(--mono);
  line-height: 1.55;
  height: 380px;
  overflow-y: auto;
  white-space: pre-wrap;
  word-break: break-all;
  color: #94a3b8;
  margin: 0;
}
.clickable { cursor: pointer; }
.clickable:hover { color: var(--accent-soft); }
</style>
