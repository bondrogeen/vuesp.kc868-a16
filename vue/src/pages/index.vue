<script setup lang="ts">
import type { IA16Device } from '@/types';

import { computed, ref } from 'vue';

import { KEYS } from '@/utils/const';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VTextField } from 'vuesp-components/components';

const { $t } = useLocale();

const { main, onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

const device = computed(() => main.value.device as IA16Device);

const inputMask = computed(() => (device.value.portIn1 << 8) | device.value.portIn2);
const outputMask = computed(() => (device.value.portOut2 << 8) | device.value.portOut1);

const isInput = (index: number) => Boolean(inputMask.value & (1 << (index - 1)));
const isOutput = (index: number) => Boolean(outputMask.value & (1 << (index - 1)));

const toggleOutput = (index: number) => {
  const bit = index - 1;
  const mask = 1 << (bit % 8);
  const next = { ...device.value };

  if (bit < 8) {
    next.portOut1 = next.portOut1 & mask ? next.portOut1 & ~mask : next.portOut1 | mask;
  } else {
    next.portOut2 = next.portOut2 & mask ? next.portOut2 & ~mask : next.portOut2 | mask;
  }

  onSend(KEYS.DEVICE, { ...next, command: 0 });
};

const message = ref('');
const sendMessage = () => {
  onSend(KEYS.DEVICE, { ...device.value, message: message.value, command: 0 });
  message.value = '';
};

const adc = computed(() => [
  { label: 'ADC 1', value: device.value.adc1 },
  { label: 'ADC 2', value: device.value.adc2 },
  { label: 'ADC 3', value: device.value.adc3 },
  { label: 'ADC 4', value: device.value.adc4 },
]);
</script>

<template>
  <div class="container mx-auto">
    <section class="mb-6 flex items-center justify-between">
      <h1>{{ $t('menu.home') }}</h1>
      <span class="text-sm text-gray-500">KC868-A16</span>
    </section>

    <div class="grid grid-cols-1 gap-4 xl:grid-cols-2">
      <card-main title="Inputs" class="p-4 sm:p-6">
        <div class="grid grid-cols-4 gap-3 sm:grid-cols-8">
          <div v-for="index of 16" :key="index" class="glass-panel flex flex-col items-center gap-1 rounded-xl px-2 py-3">
            <span class="size-2.5 rounded-full" :class="isInput(index) ? 'bg-emerald-500 shadow-[0_0_12px_rgba(16,185,129,0.9)]' : 'bg-gray-300 dark:bg-gray-700'"></span>
            <span class="text-[10px] font-semibold uppercase text-gray-500">IN {{ index }}</span>
          </div>
        </div>
      </card-main>

      <card-main title="Outputs" class="p-4 sm:p-6">
        <div class="grid grid-cols-4 gap-3 sm:grid-cols-8">
          <button v-for="index of 16" :key="index" class="glass-panel flex flex-col items-center gap-1 rounded-xl px-2 py-3 transition-all hover:brightness-110" @click="toggleOutput(index)">
            <span class="size-2.5 rounded-full" :class="isOutput(index) ? 'bg-blue-500 shadow-[0_0_12px_rgba(37,99,235,0.9)]' : 'bg-gray-300 dark:bg-gray-700'"></span>
            <span class="text-[10px] font-semibold uppercase text-gray-500">OUT {{ index }}</span>
          </button>
        </div>
      </card-main>

      <card-main title="ADC" class="p-4 sm:p-6">
        <div class="grid grid-cols-2 gap-3 sm:grid-cols-4">
          <div v-for="item in adc" :key="item.label" class="glass-panel rounded-xl px-3 py-4 text-center">
            <div class="mb-1 text-[10px] uppercase tracking-[0.2em] text-gray-500">{{ item.label }}</div>
            <div class="font-mono text-xl font-bold">{{ item.value }}</div>
          </div>
        </div>
      </card-main>

      <card-main title="Message" class="p-4 sm:p-6">
        <div class="flex flex-col gap-3 sm:flex-row">
          <VTextField v-model="message" class="flex-auto" placeholder="Message" @keyup.enter="sendMessage" />
          <VButton color="blue" :disabled="!message" @click="sendMessage">Send</VButton>
        </div>
      </card-main>
    </div>
  </div>
</template>
