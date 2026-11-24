<template>
  <div class="flex flex-col gap-3">
    <div class="flex items-start justify-between gap-4">
      <div class="min-w-0">
        <h4 class="truncate text-sm font-semibold">{{ details.title }}</h4>
        <p v-if="details.subtitle" class="mt-1 truncate font-mono text-xs text-gray-dark-25 dark:text-gray-light-10">{{ details.subtitle }}</p>
      </div>
      <VButton v-if="closable" type="icon" variant="ghost" :title="$t('modbus.close')" :aria-label="$t('modbus.close')" @click="emit('close')">
        <VIcon name="close" :size="16" />
      </VButton>
    </div>

    <div class="grid grid-cols-1 gap-x-4 gap-y-2 text-sm sm:grid-cols-2 lg:grid-cols-3">
      <div>
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.type') }}</div>
        <div class="mt-0.5 font-medium">{{ details.type }}</div>
      </div>
      <div>
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.address') }}</div>
        <div class="mt-0.5 font-medium">{{ details.address }}</div>
      </div>
      <div>
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">FC</div>
        <div class="mt-0.5 font-medium">{{ details.fc.toString().padStart(2, '0') }}</div>
      </div>
      <div>
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ details.countLabel }}</div>
        <div class="mt-0.5 font-medium">{{ details.count }}</div>
      </div>
      <div>
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.access') }}</div>
        <div class="mt-0.5 font-medium">{{ details.access }}</div>
      </div>
      <div v-if="details.hex">
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.hex') }}</div>
        <div class="mt-0.5 break-all font-mono text-xs font-medium">{{ details.hex }}</div>
      </div>
      <div v-if="details.source">
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.source') }}</div>
        <div class="mt-0.5 break-all font-mono text-xs">{{ details.source }}</div>
      </div>
      <div v-if="details.live">
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.live') }}</div>
        <div class="mt-0.5 font-medium">{{ details.live }}</div>
      </div>
    </div>

    <div>
      <div class="mb-1 flex items-center justify-between gap-2">
        <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.readFrame') }}</div>
        <span v-if="copied" class="text-xs text-green-600 dark:text-green-400">{{ $t('copied') }}</span>
      </div>
      <div class="flex items-center gap-2">
        <code class="min-w-0 flex-auto break-all rounded-md bg-black/5 px-3 py-2 font-mono text-xs dark:bg-white/5">{{ details.readFrame }}</code>
        <VButton type="icon" variant="outline" :title="$t('modbus.copyFrame')" :aria-label="$t('modbus.copyFrame')" @click="copy(details.readFrame)">
          <VIcon name="copy" :size="16" />
        </VButton>
      </div>
    </div>

    <div v-if="details.responseFrame">
      <div class="mb-1 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.responseFrame') }}</div>
      <code class="block break-all rounded-md bg-black/5 px-3 py-2 font-mono text-xs dark:bg-white/5">{{ details.responseFrame }}</code>
    </div>

    <p v-if="details.note" class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ details.note }}</p>
    <p v-if="details.notApplied" class="text-xs text-amber-600 dark:text-amber-400">{{ $t('modbus.notApplied') }}</p>
  </div>
</template>

<script setup lang="ts">
import type { IMapDetails } from './modbusMapTypes';

import { useClipboard } from '@vueuse/core';

import { VButton, VIcon } from 'vuesp-components/components';

const { details, closable = false } = defineProps<{ details: IMapDetails; closable?: boolean }>();
const emit = defineEmits<{ (e: 'close'): void }>();

const { copy, copied } = useClipboard({ copiedDuring: 1200, legacy: true });
</script>
