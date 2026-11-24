<script setup lang="ts">
import { computed, onUnmounted, ref, watch } from 'vue';

import { VCheckbox } from 'vuesp-components/components';

import { KEY } from '@/generated/protocol';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

const { $t } = useLocale();
const { main, onSend } = useConnection();

const POLL_INTERVAL_MS = 1000;

const enabled = ref(false);
let timer: ReturnType<typeof setInterval> | null = null;

const request = () => onSend(KEY.INFO, { diagnostics: 1 });

const stopTimer = () => {
  if (timer) clearInterval(timer);
  timer = null;
};

watch(enabled, (value) => {
  stopTimer();
  if (!value) {
    onSend(KEY.INFO, { diagnostics: 0 });
    return;
  }
  request();
  timer = setInterval(request, POLL_INTERVAL_MS);
});

onUnmounted(() => {
  stopTimer();
  if (enabled.value) onSend(KEY.INFO, { diagnostics: 0 });
});

const info = computed(() => main.value.info);

const kb = (value: number) => `${(value / 1024).toFixed(1)} KB`;
const us = (value: number) => `${value.toLocaleString()} µs`;
const count = (value: number) => value.toLocaleString();

const metrics = computed(() => {
  const data = info.value;
  return [
    { label: $t('diag.heap'), value: kb(data.freeHeap) },
    { label: $t('diag.heapMin'), value: kb(data.minFreeHeap) },
    { label: $t('diag.maxBlock'), value: kb(data.maxBlock) },
    { label: $t('diag.stack'), value: kb(data.stackFree) },
    { label: $t('diag.loop'), value: us(data.loopLastUs) },
    { label: $t('diag.loopP95'), value: us(data.loopP95Us) },
    { label: $t('diag.loopMax'), value: us(data.loopMaxUs) },
    { label: $t('diag.loopCount'), value: count(data.loopCount) },
    { label: $t('diag.clients'), value: count(data.clients) },
    { label: $t('diag.scripts'), value: count(data.scripts) },
    { label: $t('diag.peers'), value: count(data.peers) },
    { label: $t('diag.wifi'), value: data.wifi ? $t('homePage.online') : $t('homePage.offline') },
    { label: $t('diag.fragmentation'), value: `${data.fragmentation}%` },
  ];
});
</script>

<template>
  <div class="flex flex-col gap-2 md:flex-row md:items-center md:justify-between md:gap-4">
    <p class="text-sm leading-snug text-gray-dark-25 dark:text-gray-main">{{ $t('diag.hint') }}</p>

    <VCheckbox v-model="enabled">{{ $t('diag.enable') }}</VCheckbox>
  </div>

  <div v-if="enabled" class="mt-4 grid grid-cols-1 gap-2 sm:grid-cols-2 xl:grid-cols-3 2xl:grid-cols-4">
    <div v-for="metric of metrics" :key="metric.label" class="flex items-center justify-between gap-4 rounded-md bg-white/5 px-3 py-2">
      <span class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ metric.label }}</span>
      <span class="text-xs font-medium">{{ metric.value }}</span>
    </div>
  </div>
</template>
