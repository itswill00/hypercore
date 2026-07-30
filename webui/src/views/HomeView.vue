<template>
  <div>
    <ProfileSelector />
    <StatusCard />
    <ActionButtons />
    <GameList />
    
    <div class="card" style="margin-top: 12px;">
      <div class="card-title">
        <span>Daemon log</span>
        <router-link to="/logs" class="badge clickable">Full View</router-link>
      </div>
      <pre class="log-preview">{{ store.logs }}</pre>
    </div>
  </div>
</template>

<script setup>
import ProfileSelector from '@/components/ProfileSelector.vue'
import StatusCard from '@/components/StatusCard.vue'
import ActionButtons from '@/components/ActionButtons.vue'
import GameList from '@/components/GameList.vue'
import { useHyperStore } from '@/stores/hyper'
import { watch, nextTick } from 'vue'

const store = useHyperStore()

watch(() => store.logs, async () => {
  await nextTick()
  const el = document.querySelector('.log-preview')
  if (el) el.scrollTop = el.scrollHeight
})
</script>

<style scoped>
.log-preview {
  background: var(--surface-alt);
  border: 1px solid var(--border);
  padding: 8px 10px;
  border-radius: var(--radius-sm);
  font-size: 10px;
  font-family: var(--mono);
  line-height: 1.5;
  height: 160px;
  overflow-y: auto;
  white-space: pre-wrap;
  word-break: break-all;
  color: #94a3b8;
  margin: 0;
}
.clickable {
  cursor: pointer;
  text-decoration: none;
  color: var(--accent-soft);
}
.clickable:hover { text-decoration: underline; }
</style>
