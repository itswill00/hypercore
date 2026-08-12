<template>
  <Teleport to="body">
    <Transition name="modal">
      <div v-if="show" class="modal-backdrop" @click.self="close">
        <div class="modal-window" role="dialog" aria-modal="true" aria-labelledby="br-title" @click.stop>

          <!-- Header -->
          <div class="modal-header">
            <div>
              <div class="modal-title" id="br-title">Bug Report</div>
              <div class="modal-sub">Describe your issue before exporting</div>
            </div>
            <button class="modal-close" @click="close" aria-label="Close">✕</button>
          </div>

          <!-- Note field -->
          <div class="field-label">What happened? <span class="optional-tag">optional</span></div>
          <textarea
            v-model="note"
            class="note-input"
            placeholder="e.g. Gaming profile not applying on MLBB, CPU stays at min freq..."
            rows="3"
            maxlength="500"
            :disabled="generating"
          ></textarea>
          <div class="char-count">{{ note.length }}/500</div>

          <!-- What's included -->
          <div class="includes-title">What's included</div>
          <div class="includes-grid">
            <div class="include-chip" v-for="item in includes" :key="item">
              <span class="include-dot"></span>{{ item }}
            </div>
          </div>

          <!-- State: idle / generating / done -->
          <div v-if="!resultPath && !generating" class="action-row">
            <button class="btn-cancel" @click="close">Cancel</button>
            <button class="btn-generate" @click="generate">
              <Icons name="logs" :size="15" />
              Generate Report
            </button>
          </div>

          <div v-else-if="generating" class="generating-state">
            <div class="spinner-wrap">
              <div class="spinner"></div>
            </div>
            <div class="gen-label">Collecting system data...</div>
            <div class="gen-sub">{{ generatingSteps[stepIdx] }}</div>
          </div>

          <div v-else-if="resultPath" class="result-state">
            <div class="result-icon">✓</div>
            <div class="result-label">Report saved</div>
            <div class="result-path" @click="copyPath" :title="resultPath">
              {{ shortPath(resultPath) }}
              <span class="copy-hint">tap to copy</span>
            </div>
            <div class="action-row" style="margin-top: 14px;">
              <button class="btn-cancel" @click="close">Done</button>
              <button class="btn-generate secondary" @click="reset">New Report</button>
            </div>
          </div>

        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<script setup>
import { ref, inject, onMounted, onUnmounted } from 'vue'
import { useHyperStore } from '@/stores/hyper'
import Icons from '@/components/icons/Icons.vue'

const props = defineProps({
  show: Boolean
})
const emit = defineEmits(['close'])

const store = useHyperStore()
const toast = inject('toast')

const note = ref('')
const generating = ref(false)
const resultPath = ref('')
const stepIdx = ref(0)
let stepTimer = null

const generatingSteps = [
  'Reading daemon log...',
  'Dumping sysfs nodes...',
  'Capturing process snapshot...',
  'Collecting logcat...',
  'Packing archive...',
]

const includes = [
  'Daemon log',
  'Sysfs nodes',
  'Process list',
  'Logcat tail',
  'Device info',
  'Game list',
  'Your note',
]

function close() {
  if (generating.value) return
  emit('close')
}

function reset() {
  note.value = ''
  resultPath.value = ''
  stepIdx.value = 0
}

function shortPath(p) {
  if (!p) return ''
  const parts = p.split('/')
  return '…/' + parts.slice(-2).join('/')
}

async function copyPath() {
  try {
    await navigator.clipboard.writeText(resultPath.value)
    if (toast) toast('Path copied')
  } catch {
    if (toast) toast(resultPath.value)
  }
}

async function generate() {
  generating.value = true
  stepIdx.value = 0
  resultPath.value = ''

  // Cycle through step labels for UX feedback
  stepTimer = setInterval(() => {
    stepIdx.value = (stepIdx.value + 1) % generatingSteps.length
  }, 900)

  try {
    const path = await store.exportLogs(note.value)
    generating.value = false
    clearInterval(stepTimer)
    if (path) {
      resultPath.value = path
    } else {
      if (toast) toast('Failed to generate report')
      emit('close')
    }
  } catch {
    generating.value = false
    clearInterval(stepTimer)
    if (toast) toast('Failed to generate report')
    emit('close')
  }
}

onUnmounted(() => {
  clearInterval(stepTimer)
})
</script>

<style scoped>
.modal-backdrop {
  position: fixed;
  inset: 0;
  height: 100dvh;
  max-height: 100dvh;
  background: rgba(10, 11, 13, 0.78);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  z-index: 999;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding: 16px;
}

.modal-window {
  width: 100%;
  max-width: 440px;
  background: var(--surface-container-high, #26272e);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 20px;
  padding: 18px 20px 20px;
  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.55);
  max-height: calc(100dvh - 32px);
  overflow-y: auto;
  overscroll-behavior: contain;
  margin: auto;
}

.modal-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 18px;
  padding-bottom: 14px;
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
  margin-top: 2px;
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
  flex-shrink: 0;
  transition: transform 0.12s ease;
}

.modal-close:active { transform: scale(0.88); }

.field-label {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant, #b0b4c0);
  margin-bottom: 7px;
  display: flex;
  align-items: center;
  gap: 6px;
}

.optional-tag {
  font-weight: 400;
  opacity: 0.55;
  font-size: 10px;
}

.note-input {
  width: 100%;
  background: var(--surface-container, #1d1e23);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 12px;
  padding: 11px 13px;
  font-size: 12.5px;
  font-family: inherit;
  color: var(--on-surface, #f0f2f5);
  resize: none;
  outline: none;
  line-height: 1.55;
  transition: border-color 0.15s ease;
  box-sizing: border-box;
}

.note-input:focus {
  border-color: var(--primary, #8b8fcf);
}

.note-input::placeholder {
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.45;
}

.note-input:disabled {
  opacity: 0.4;
  pointer-events: none;
}

.char-count {
  text-align: right;
  font-size: 10px;
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.45;
  margin-top: 4px;
  font-family: var(--font-mono);
  margin-bottom: 16px;
}

.includes-title {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant, #b0b4c0);
  margin-bottom: 8px;
}

.includes-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-bottom: 20px;
}

.include-chip {
  display: flex;
  align-items: center;
  gap: 5px;
  background: var(--surface-container, #1d1e23);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 8px;
  padding: 3px 9px;
  font-size: 11px;
  color: var(--on-surface-variant, #b0b4c0);
}

.include-dot {
  width: 5px;
  height: 5px;
  border-radius: 50%;
  background: var(--primary, #8b8fcf);
  flex-shrink: 0;
}

/* Action row */
.action-row {
  display: flex;
  gap: 10px;
}

.btn-cancel {
  flex: 1;
  padding: 11px;
  border-radius: 12px;
  border: 1px solid var(--outline-variant, #383a42);
  background: transparent;
  color: var(--on-surface-variant, #b0b4c0);
  font-size: 13px;
  font-weight: 600;
  font-family: inherit;
  cursor: pointer;
  transition: background 0.12s ease, transform 0.1s ease;
}

.btn-cancel:active {
  transform: scale(0.96);
  background: var(--surface-container, #1d1e23);
}

.btn-generate {
  flex: 2;
  padding: 11px 16px;
  border-radius: 12px;
  border: none;
  background: var(--primary-container, #33353e);
  color: var(--on-primary-container, #d0d2f5);
  font-size: 13px;
  font-weight: 700;
  font-family: inherit;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 7px;
  transition: filter 0.15s ease, transform 0.1s ease;
}

.btn-generate:active {
  transform: scale(0.96);
  filter: brightness(0.9);
}

.btn-generate.secondary {
  flex: 1;
  background: var(--surface-container, #1d1e23);
  color: var(--on-surface, #f0f2f5);
  border: 1px solid var(--outline-variant, #383a42);
}

/* Generating state */
.generating-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  padding: 20px 0 8px;
}

.spinner-wrap {
  width: 44px;
  height: 44px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.spinner {
  width: 28px;
  height: 28px;
  border: 2.5px solid var(--outline-variant, #383a42);
  border-top-color: var(--primary, #8b8fcf);
  border-radius: 50%;
  animation: spin 0.75s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.gen-label {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface, #f0f2f5);
}

.gen-sub {
  font-size: 11px;
  color: var(--on-surface-variant, #b0b4c0);
  opacity: 0.7;
  font-family: var(--font-mono);
  min-height: 16px;
  transition: opacity 0.2s ease;
}

/* Result state */
.result-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
  padding: 8px 0;
}

.result-icon {
  width: 44px;
  height: 44px;
  border-radius: 50%;
  background: rgba(100, 200, 130, 0.15);
  border: 1.5px solid rgba(100, 200, 130, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  color: #7dd8a0;
  margin-bottom: 2px;
}

.result-label {
  font-size: 14px;
  font-weight: 700;
  color: var(--on-surface, #f0f2f5);
}

.result-path {
  font-size: 11px;
  font-family: var(--font-mono);
  color: var(--on-surface-variant, #b0b4c0);
  background: var(--surface-container, #1d1e23);
  border: 1px solid var(--outline-variant, #383a42);
  border-radius: 8px;
  padding: 6px 12px;
  cursor: pointer;
  text-align: center;
  display: flex;
  align-items: center;
  gap: 6px;
  transition: background 0.12s ease;
}

.result-path:active {
  background: var(--surface-container-high, #26272e);
}

.copy-hint {
  font-size: 9.5px;
  opacity: 0.45;
  font-family: inherit;
}

/* Modal Transition */
.modal-enter-active {
  transition: opacity 0.3s cubic-bezier(0.32, 0.72, 0, 1);
}
.modal-leave-active {
  transition: opacity 0.22s cubic-bezier(0.32, 0.72, 0, 1);
}
.modal-enter-active .modal-window {
  transition: transform 0.35s cubic-bezier(0.32, 0.72, 0, 1), opacity 0.35s ease;
}
.modal-leave-active .modal-window {
  transition: transform 0.22s cubic-bezier(0.32, 0.72, 0, 1), opacity 0.22s ease;
}
.modal-enter-from { opacity: 0; }
.modal-enter-from .modal-window {
  opacity: 0;
  transform: translate3d(0, 50px, 0) scale(0.96);
}
.modal-leave-to { opacity: 0; }
.modal-leave-to .modal-window {
  opacity: 0;
  transform: translate3d(0, 30px, 0) scale(0.97);
}
</style>
