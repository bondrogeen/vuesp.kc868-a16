<script setup lang="ts">
import { useDebounceFn } from '@vueuse/core';
import { KEY } from '@/utils/const';
import { computed, ref } from 'vue';

import { ScriptType, examples } from '@/assets/js/script';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';
import { useLogs } from '@/composables/script/useLogs';
import { useScripts } from '@/composables/script/useScripts';
import { useSlots } from '@/composables/script/useSlots';
import { normalizeScript } from 'vuesp-components/helpers';
import { useFiles } from '@/composables/useFiles';

import { VIcon, VFile, ScriptEditor, ScriptViewDocs, VTooltip, ScriptDialog, VCard } from 'vuesp-components/components';

const { $t } = useLocale();

const { suggestions, message, main, onSend } = useConnection((send) => {
  send(KEY.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT });
});

const { logs, onHover } = useLogs(message);
const { slotInfo, currentSlot, onSlot, getColorSlot } = useSlots(main);
const { PATH, ids, content, scripts, idScript, selectedScript, addScript, onRemove, onSelect, onSaveScript, isScriptSave, onExample, onLoad } = useScripts();
const fullPath = computed(() => '/');
const onUpdate = () => {
  onLoad();
};
const { createLink, onUpload, onClickUpload } = useFiles(fullPath, onUpdate);

const dialogAdd = ref(false);
const onAddScriptDialog = () => {
  const getId = Array.from({ length: 256 }, (_, i) => i).find((i) => !ids.value.includes(i));
  idScript.value = getId || 0;
  dialogAdd.value = true;
};

const dialogViewDocs = ref(false);

const onUpdateScript = useDebounceFn(() => onSend(KEY.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT }), 1000);

const onRunScript = () => {
  const id = selectedScript.value?.id || 0;
  onSend(KEY.MESSAGE, { ...message.value, id, type: ScriptType.SCRIPT_START, text: content.value });
  onUpdateScript();
};

const onStopScript = () => {
  onSend(KEY.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_STOP, text: '' });
  onUpdateScript();
};

const onStopScriptAll = () => {
  onSend(KEY.MESSAGE, { ...message.value, type: ScriptType.SCRIPT_STOP_ALL, text: '' });
  onUpdateScript();
};

const onRemoveScript = () => {
  onSend(KEY.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_REMOVE, text: '' });
  onUpdateScript();
};
</script>

<template>
  <div>
    <div class="flex flex-col gap-6 mx-auto">
      <div class="flex justify-between">
        <h1>{{ $t('menu.script') }}</h1>

        <div>
          <v-button @click="dialogViewDocs = true" variant="ghost">
            <VIcon name="document" class="size-5" />
          </v-button>
        </div>
      </div>

      <div
        class="grid [grid-template-areas:'editor''logs''list'] xl:[grid-template-areas:'list_editor_editor_editor_editor''logs_editor_editor_editor_editor'] xl:grid-rows-[370px_1fr] grid-cols-1 xl:grid-cols-[400px_1fr_1fr_1fr] gap-3 sm:gap-4"
      >
        <VCard :title="$t('list')" class="order-2 md:order-1 [grid-area:list]" hover>
          <template #header>
            <div class="flex gap-3">
              <v-button class="size-6" size="none" variant="ghost" :title="$t('export')" :disabled="false" @click="createLink(PATH)">
                <VIcon name="upload" class="size-4"></VIcon>
              </v-button>

              <v-button class="size-6" size="none" variant="ghost" :title="$t('import')" :disabled="false" @click="onClickUpload()">
                <VIcon name="download" class="size-4"></VIcon>
              </v-button>

              <v-button class="size-6" size="none" variant="ghost" :title="$t('add')" :disabled="false" @click="onAddScriptDialog()">
                <VIcon name="add" class="size-5"></VIcon>
              </v-button>
              <VFile accept=".txt" @change="onUpload($event, PATH)"></VFile>
            </div>
          </template>

          <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-light-10 dark:border-gray-dark-30 bg-gray-light-30 dark:bg-gray-dark-40 p-2 h-36 scrollbar">
            <li v-for="script of scripts" :key="script.id" class="group text-sm cursor-pointer transition-all flex items-center justify-between hover:opacity-80 px-1" @click="onSelect(script)">
              <div class="flex items-center gap-2 min-w-0">
                <span>#{{ script.id }}</span>
                <span class="truncate">{{ script.name }}</span>
              </div>

              <div class="group flex items-center gap-1">
                <v-button
                  class="size-6"
                  size="none"
                  :class="isScriptSave(script) ? 'text-gray-dark-25' : 'text-blue-500 animate-shake'"
                  :title="$t('save')"
                  :disabled="isScriptSave(script)"
                  @click="onSaveScript"
                  variant="ghost"
                >
                  <VIcon name="save" class="size-4"></VIcon>
                </v-button>
                <v-button class="size-6" size="none" :title="$t('remove')" @click.stop="onRemove(script)" variant="ghost">
                  <VIcon name="trash" class="size-4 transition-all"></VIcon>
                </v-button>
              </div>
            </li>

            <li v-if="scripts.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
          </ul>

          <div class="px-2 border-t border-gray-light-10 dark:border-gray-dark-30 text-slate-400 flex justify-between mt-4 text-xs">
            <span>{{ scripts.length }} {{ $t('script', { count: `${scripts.length}` }) }}</span>
          </div>

          <div class="mt-4">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold text-gray-dark-30 dark:text-white/90 mb-2">{{ $t('slots') }}</h3>

              <div class="flex gap-2">
                <v-button class="size-6" variant="ghost" size="none" :disabled="!currentSlot?.active" @click="onStopScript">
                  <VIcon name="stop-circle" class="size-5"></VIcon>
                </v-button>

                <v-button class="size-6" variant="ghost" size="none" :disabled="!currentSlot?.text" @click="onRemoveScript">
                  <VIcon name="close" class="size-5"></VIcon>
                </v-button>

                <v-button class="size-6" variant="ghost" size="none" :title="$t('update')" @click="onUpdateScript">
                  <VIcon name="refresh" class="size-5"></VIcon>
                </v-button>

                <v-button class="size-6" variant="ghost" size="none" :title="$t('stop')" :disabled="!slotInfo.used" @click="onStopScriptAll">
                  <VIcon name="stop-circle" class="size-5"></VIcon>
                </v-button>
              </div>
            </div>

            <ul class="flex flex-wrap gap-1 mb-2">
              <li v-for="slot in main.slots" :key="slot.index" class="group text-sm transition-all flex items-center rounded">
                <VTooltip>
                  <template #activator>
                    <v-button class="px-0! rounded-none" :class="getColorSlot(slot)" size="none" @click="onSlot(slot)" variant="none">
                      <div class="flex-[0_0_8px] h-4 w-2"></div>
                    </v-button>
                  </template>
                  <template #default="{ show }">
                    <div v-if="show" class="text-sm absolute bottom-full left-0 bg-white-main px-3 py-2 rounded shadow-nav z-5 w-50 bg-white dark:bg-gray-dark-30">
                      <div class="text-slate-400">{{ slot?.id === 255 ? '' : `id:${slot?.id}` }} ({{ slot.active ? 'active' : slot.handler ? 'event' : slot?.id === 255 ? 'empty' : 'stopped' }})</div>

                      <p>{{ slot?.text }}</p>
                    </div>
                  </template>
                </VTooltip>
              </li>
            </ul>
          </div>

          <div class="px-2 border-t border-gray-light-10 dark:border-gray-dark-30 text-slate-400 flex text-xs">
            <span>{{ `${$t('total')}: ${slotInfo.total} ${$t('used')}: ${slotInfo.used}` }}</span>
          </div>
        </VCard>

        <VCard :title="`${$t('editor')}  ${selectedScript?.name ? selectedScript?.name : ''}`" class="order-1 [grid-area:editor]" hover>
          <template #header>
            <div class="flex gap-3">
              <v-button
                color="green"
                variant="soft"
                class="size-6"
                size="none"
                :title="$t('run')"
                :disabled="!Boolean(content.trim()) || !selectedScript?.name"
                @click="onRunScript()"
              >
                <VIcon name="play-circle" class="size-4" />
              </v-button>

              <v-select :items="examples" @change="onExample">
                <template #activator="{ on }">
                  <v-button class="size-6" size="none" :disabled="!selectedScript" @click="on.click" variant="ghost">
                    <VIcon name="file-list" class="size-4"></VIcon>
                  </v-button>
                </template>
              </v-select>
            </div>
          </template>

          <div class="flex-auto bg-gray-light-30 dark:bg-gray-dark-40 dark:border-gray-dark-30 w-full overflow-auto scrollbar h-50 xl:h-[calc(100dvh-300px)]">
            <ScriptEditor v-if="selectedScript?.name" v-model="content" class="h-full w-full overflow-auto scrollbar" :suggestions="suggestions"></ScriptEditor>
            <p v-else class="text-gray-dark-25 ms-3 text-center mt-4">{{ $t('selectScript') }}</p>
          </div>

          <div class="text-sm border-t border-gray-light-10 dark:border-gray-dark-30 text-slate-400 flex flex-wrap justify-between mt-4">
            <span>
              <i class="far fa-file-alt mr-1"></i>
              {{ `${$t('length')}: ${normalizeScript(content)?.length || 0}` }}
            </span>
          </div>
        </VCard>

        <VCard :title="$t('logs')" class="order-1 [grid-area:logs]" hover>
          <template #header>
            <v-button class="size-6 opacity-60 hover:opacity-100" size="none" @click="logs = []" variant="ghost">
              <VIcon name="eraser" class="size-5 text-gray-dark-25"></VIcon>
            </v-button>
          </template>

          <ul
            ref="container"
            class="overflow-y-auto flex flex-col rounded-md border border-gray-light-10 dark:border-gray-dark-30 bg-gray-light-30 dark:bg-gray-dark-40 p-4 max-h-50 min-h-50 scrollbar"
            @mouseenter="onHover(true)"
            @mouseleave="onHover(false)"
          >
            <li v-for="(log, idx) in logs" :key="idx" class="py-0.5 flex items-start gap-2 border-b border-slate-600/50 last:border-0 text-sm">
              <span class="text-slate-400 whitespace-nowrap">{{ log.time }}</span>
              <span>
                {{ log.text }}
              </span>
            </li>
            <li v-if="logs.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noLogs') }}</li>
          </ul>
        </VCard>
      </div>
    </div>

    <v-dialog v-if="dialogAdd" size="sm" :title="`${$t('add')} #${idScript}`" @close="dialogAdd = false">
      <ScriptDialog :idScript="idScript" @add="addScript" @close="dialogAdd = false" />
    </v-dialog>

    <v-dialog v-if="dialogViewDocs" size="lg" @close="dialogViewDocs = false">
      <ScriptViewDocs class="min-h-100" />
    </v-dialog>
  </div>
</template>
