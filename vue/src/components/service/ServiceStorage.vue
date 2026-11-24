<script setup lang="ts">
import type { Ref } from 'vue';
import type { IListItem, IFiles, TMessage } from '@/types';
import { KEY, CMD } from '@/generated/protocol';

import { watchEffect, ref, computed, nextTick } from 'vue';
import { useDebounceFn } from '@vueuse/core';
import { toByte } from 'vuesp-components/helpers';
import { useFiles } from '@/composables/useFiles';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VIcon, VFile, VCard } from 'vuesp-components/components';

const { $t } = useLocale();

const mainMenu: IListItem<number>[] = [
  { name: $t('upload'), value: 2 },
  { name: $t('reload'), value: 3 },
  { name: $t('format'), value: 4 },
];
const listMenu: IListItem<number>[] = [
  { name: $t('download'), value: 1 },
  { name: $t('remove'), value: 2 },
];

const isLoading = ref(false);
const path = ref(['root']);
const files: Ref<IFiles[]> = ref([]);
const fullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);
const sortFiles = computed<IFiles[]>(() => JSON.parse(JSON.stringify(files.value)).sort((a: IFiles, b: IFiles) => ((a.isFile || 0) > (b.isFile || 0) ? 1 : -1)));

const getListMenu = (isDir: boolean) => listMenu.filter((i) => (isDir ? i.value !== 1 : true));
const isLast = (path: string[], i: number) => path.length > i + 1;

const onMessage = ({ key, object }: TMessage) => {
  if (key === KEY.FILES && object) files.value.push(object);
};

const { onSend, onDialog } = useConnection((send) => {
  send(KEY.FILES, { command: CMD.GET, name: fullPath.value });
}, onMessage);

const onUpdate = () => {
  isLoading.value = true;
  files.value = [];
  onSend(KEY.FILES, { command: CMD.GET, name: fullPath.value });
  onSend(KEY.INFO);
};

const { fileName, createLink, onDelete, onUpload, onFormat, onClickUpload } = useFiles(fullPath, onUpdate);

const onPrev = (index: number) => {
  if (path.value.length > index + 1) {
    path.value = path.value.filter((_, i) => i < index + 1);
    nextTick(() => onUpdate());
  }
};

const onNext = (isDir: boolean, value: string) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onEventService = ({ value }: IListItem<number>) => {
  if (value === 2) onClickUpload();
  if (value === 3) onUpdate();
  if (value === 4) onSureFormat();
};

const onSureFormat = () => onDialog({ value: true, message: $t('dialog.allDel'), callback: onFormat });

const onEventList = (name: string, { value }: IListItem<number>) => {
  if (value === 1) createLink(name);
  if (value === 2) onSureDelete(name);
};

const onSureDelete = (name: string) => {
  if (fileName(name).includes('www')) {
    onDialog({ value: true, message: $t('dialog.wwwDir'), callback: onDelete.bind(this, name) });
  } else {
    onDelete(name);
  }
};

const onLoad = useDebounceFn((_?: IFiles[]) => {
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(files.value);
});
</script>

<template>
  <div class="grid grid-cols-1 xl:grid-cols-2">
    <VCard :title="$t('fb')" hover>
      <div class="px-4 pb-2 flex items-center">
        <div class="flex gap-2 items-center font-bold flex-auto">
          <div
            v-for="(value, i) of path"
            :key="value"
            class="flex items-center gap-2 cursor-pointer text-gray-dark-25 last:cursor-default last:text-gray-dark-40 dark:text-gray-light-10 dark:last:text-white"
            @click="onPrev(i)"
          >
            <div>{{ value }}</div>

            <VIcon name="arrow-right" v-if="isLast(path, i)" class="h-5 w-4"></VIcon>
          </div>
        </div>

        <VFile multiple="multiple" @change="onUpload"></VFile>
      </div>

      <div class="relative min-h-[260px]">
        <v-loader v-if="isLoading" class="absolute top-1/2 left-1/2 text-cyan-main" />

        <v-list v-slot="{ item: { name, size, isDir, isFile } }" :items="sortFiles">
          <div class="flex items-center flex-auto" @click="onNext(Boolean(isDir), name)">
            <div class="mr-4 text-gray-main">
              <VIcon name="folder" v-if="isDir" class="size-4"></VIcon>
              <VIcon name="file" v-else class="size-4"></VIcon>
            </div>

            <div>
              <div class="text-sm">{{ isDir ? `${name}` : name }}</div>

              <div v-if="isFile" class="text-sm text-xs text-gray-main">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>

          <v-select :items="getListMenu(Boolean(isDir))" @change="onEventList(name, $event)">
            <template #activator="{ on }">
              <v-button @click="on.click" variant="ghost">
                <VIcon name="more" class="size-5 rotate-90"></VIcon>
              </v-button>
            </template>
          </v-select>
        </v-list>
      </div>
    </VCard>

    <Teleport to="#device-slot">
      <v-select :items="mainMenu" @change="onEventService">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <VIcon name="more" class="size-5 rotate-90"></VIcon>
          </v-button>
        </template>
      </v-select>
    </Teleport>
  </div>
</template>
