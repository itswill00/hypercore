<template>
  <div style="height: 100%; display: flex; flex-direction: column;">
    <!-- Sticky Header -->
    <div class="page-header">
      <div>
        <div class="page-header-title">Dashboard</div>
        <div class="page-header-sub">HyperCore By @itswill00</div>
      </div>
      <button class="btn-md3 btn-md3-secondary btn-icon-only" @click="store.refresh()">
        <Icons name="refresh" :size="16" />
      </button>
    </div>

    <!-- Scroll Content -->
    <div class="content-area">
      <ProfileSelector />
      <StatusCard />
      <ActionButtons />
    </div>
  </div>
</template>

<script setup>
import { useHyperStore } from '@/stores/hyper'
import ProfileSelector from '@/components/ProfileSelector.vue'
import StatusCard from '@/components/StatusCard.vue'
import ActionButtons from '@/components/ActionButtons.vue'
import Icons from '@/components/icons/Icons.vue'

const store = useHyperStore()
</script>
