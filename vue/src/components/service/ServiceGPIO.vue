<script setup lang="ts">
import type { IListItem, IPort } from '@/types';
import { KEY } from '@/utils/const';

import { CMD, PORT } from '@/generated/protocol';

import { useDebounceFn } from '@vueuse/core';

import { useConnection } from '@/composables/useConnection';
import { useFetch } from '@vueuse/core';
import { PATH_FS, pathGPIO } from '@/utils/const';
import { useLocale } from '@/composables/useLocale';
import { VCard, VIcon, VCheckbox, VTooltip } from 'vuesp-components/components';

const { $t } = useLocale();
const { main, onSend, onDialog } = useConnection((send) => {
  send(KEY.PORT, { gpio: 0, command: CMD.GET_ALL });
});

const listMenu: IListItem<number>[] = [
  { name: $t('save'), value: 1 },
  { name: $t('restore'), value: 2 },
];

const modes: Array<{ name: string; value: number }> = [
  { name: 'INPUT', value: PORT.INPUT },
  { name: 'INPUT_PULLUP', value: PORT.INPUT_PULLUP },
  { name: 'OUTPUT', value: PORT.OUTPUT },
  { name: 'OUTPUT_OPEN_DRAIN', value: PORT.OUTPUT_OPEN_DRAIN },
  { name: 'PWM', value: PORT.PWM },
  { name: 'ONEWIRE', value: PORT.ONEWIRE },
  { name: 'ADC', value: PORT.ADC },
  { name: 'DAC', value: PORT.DAC },
];

const getListMode = (port: IPort): IListItem<number>[] =>
  modes.filter((mode) => ((port?.list || 0) & mode.value) !== 0).map(({ name, value }) => ({ name, value }));

const getNameList = (port: IPort) => getListMode(port).find((i) => i.value === port.mode)?.name ?? '';
const getNameListInterrupt = (port: IPort) => listInterrupt.find((i) => i.value === port.interrupt)?.name ?? '';

const listInterrupt: IListItem<number>[] = [
  { name: 'OFF', value: 0 },
  { name: 'RISING', value: 1 },
  { name: 'FALLING', value: 2 },
  { name: 'CHANGE', value: 3 },
];

const isOutput = ({ mode = 0 }: IPort) => mode === PORT.OUTPUT || mode === PORT.OUTPUT_OPEN_DRAIN;
const isInput = ({ mode = 0 }: IPort) => mode === PORT.INPUT || mode === PORT.INPUT_PULLUP;
const isPWM = ({ mode = 0 }: IPort) => mode === PORT.PWM;
const isADC = ({ mode = 0 }: IPort) => mode === PORT.ADC;
const isDAC = ({ mode = 0 }: IPort) => mode === PORT.DAC;

const onSetPort = (port: IPort, value: number) => onSend(KEY.PORT, { ...port, command: CMD.SET, value });
const onInputValue = useDebounceFn((port: IPort, value: string) => onSend(KEY.PORT, { ...port, command: CMD.SET, value: +value }), 500);

const onMode = ({ gpio }: IPort, { value }: IListItem<number>) => {
  main.value.ports[gpio].mode = value;
  onSend(KEY.PORT, { ...main.value.ports[gpio], command: CMD.CHANGE });
};

const onInterrupt = ({ gpio }: IPort, { value }: IListItem<number>) => {
  main.value.ports[gpio].interrupt = value;
  onSend(KEY.PORT, { ...main.value.ports[gpio], command: CMD.CHANGE });
};

const onIsButton = ({ gpio }: IPort, value: boolean) => {
  main.value.ports[gpio].isButton = value ? 1 : 0;
  onSend(KEY.PORT, { ...main.value.ports[gpio], command: CMD.CHANGE });
};

const onReboot = () => onSend(KEY.REBOOT);
const onSureReboot = () => onDialog({ value: true, message: $t('dialog.doRestart'), callback: onReboot });

const onSave = async () => {
  onSend(KEY.PORT, { gpio: 0, command: CMD.SAVE });
  onSureReboot();
};
const onRestore = async () => {
  const { data } = await useFetch(`${PATH_FS}?file=${pathGPIO}`).delete().json();
  if (data.value?.state) onReboot();
};

const onMenu = ({ value }: IListItem<number>) => {
  if (value === 1) onSave();
  if (value === 2) onRestore();
};
</script>

<template>
  <div>
    <div class="relative grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 2xl:grid-cols-4 gap-4">
        <VCard v-for="port in main.ports" :key="port.gpio" hover variant="glass" padding="lg" class="relative has-[.z-30]:z-20">
          <div class="mb-2">
            <h5 class="text-sm">{{ `GPIO: ${port.gpio} ${port.disabled ? `(${$t('dis')})` : ''}` }}</h5>
          </div>

          <div class="flex flex-col gap-4">
            <div class="grid gap-4 flex-auto grid-cols-1">
              <v-select :model-value="getNameList(port)" :label="$t('mode')" :disabled="Boolean(port.disabled)" :items="getListMode(port)" @change="onMode(port, $event)" />

              <v-select
                v-if="isInput(port)"
                :model-value="getNameListInterrupt(port)"
                :disabled="Boolean(port.disabled)"
                :label="$t('interrupt')"
                :items="listInterrupt"
                @change="onInterrupt(port, $event)"
              />

              <div v-if="isInput(port) && getNameListInterrupt(port) === 'CHANGE'" class="flex items-center justify-between">
                <VCheckbox :model-value="Boolean(port.isButton)" @update:model-value="onIsButton(port, $event)">{{ $t('isButton') }}</VCheckbox>

                <VTooltip>
                  <template #activator>
                    <VIcon name="info" class="size-4 text-gray-dark-25" />
                  </template>
                  <template #default="{ show }">
                    <div v-if="show" class="absolute top-0 right-0 bg-white-main px-3 py-2 rounded shadow-nav z-5 w-70 bg-white dark:bg-gray-dark-30">
                      <p>{{ $t('message.shortPress') }}</p>
                    </div>
                  </template>
                </VTooltip>
              </div>
            </div>

            <div class="grid grid-cols-2 gap-4">
              <v-button v-if="isOutput(port)" color="blue" class="w-full" :disabled="!port.mode || Boolean(port.disabled)" @click="onSetPort(port, port.value ? 0 : 1)">
                {{ port.value ? $t('on') : $t('off') }}
              </v-button>

              <v-text-field
                v-if="isInput(port) || isPWM(port) || isADC(port) || isDAC(port)"
                hideMessage
                :modelValue="port.value"
                :disabled="isInput(port) || Boolean(port.disabled)"
                :label="$t('value')"
                @update:modelValue="onInputValue(port, $event)"
              ></v-text-field>
            </div>
          </div>
        </VCard>
      </div>

    <Teleport to="#device-slot">
      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <VIcon name="more" class="size-5 rotate-90"></VIcon>
          </v-button>
        </template>
      </v-select>
    </Teleport>
  </div>
</template>
