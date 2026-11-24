<script setup lang="ts">
import type { Ref } from 'vue';
import type { TMessage, IScan, IListItem, TSend } from '@/types';

import { computed, ref, nextTick, watch, onUnmounted } from 'vue';
import { required, maxLen, minLen, sameAs, ip, minVal, maxVal } from 'vuesp-components/validate';

import { KEY } from '@/generated/protocol';

import { useForm } from 'vuesp-components/composables';
import type { TValidationSchema } from 'vuesp-components/types';

import { useConnection } from '@/composables/useConnection';
import { arrToStr, strToArr } from 'vuesp-components/helpers';
import { useLocale } from '@/composables/useLocale';
import { useFetch } from '@vueuse/core';

import { VIcon, VCheckbox, BlockScan, BlockService, BlockUpdate, VCard } from 'vuesp-components/components';
import BlockDiagnostics from './BlockDiagnostics.vue';

const { $t, getLocale, setLocale, list } = useLocale();

const listLocale = computed(() => list.map((i: string) => ({ name: $t(`locale.${i}`), value: i })));
const onLocale = ({ value }: IListItem<string>) => setLocale(value);

const listMenu: IListItem<number>[] = [{ name: $t('save'), value: 1 }];

const isPass = ref(false);
const isAuthPass = ref(false);
const isDialog = ref(false);

const scanList: Ref<Partial<IScan>[]> = ref([]);

let settingsTimer: ReturnType<typeof setInterval> | null = null;
let infoTimer: ReturnType<typeof setInterval> | null = null;
const stopSettingsRequest = () => {
  if (settingsTimer) clearInterval(settingsTimer);
  if (infoTimer) clearInterval(infoTimer);
  settingsTimer = null;
  infoTimer = null;
};

const onMessage = ({ key, object }: TMessage) => {
  if (key === KEY.SETTINGS) stopSettingsRequest();
  if (key === KEY.SCAN && object) scanList.value.push(object);
};

const onInit = (send: TSend) => {
  send(KEY.SETTINGS);
  send(KEY.INFO);
  if (!infoTimer) {
    infoTimer = setInterval(() => send(KEY.INFO), 2000);
  }
  if (settingsTimer) return;
  let attempts = 0;
  settingsTimer = setInterval(() => {
    attempts += 1;
    if (attempts > 5) return stopSettingsRequest();
    send(KEY.SETTINGS);
  }, 2000);
};

onUnmounted(stopSettingsRequest);

const { settings, main, dialogInfo, onSend, onDialog } = useConnection(onInit, onMessage);

const { defineField, handleSubmit } = useForm({
  validationSchema: (values) =>
    ({
      wifiSsid: [required, maxLen(32)],
      wifiPass: values.wifiPass !== '' ? [required, minLen(8), maxLen(32)] : [],
      rePassword: values.wifiPass !== '' ? [required, minLen(8), maxLen(32), sameAs('wifiPass')] : [],
      wifiIp: [ip],
      wifiSubnet: [ip],
      wifiGateway: [ip],
      wifiDns: [ip],
      ethIp: settings.value.ethDhcp ? [] : [required, ip],
      ethSubnet: settings.value.ethDhcp ? [] : [required, ip],
      ethGateway: settings.value.ethDhcp ? [] : [required, ip],
      ethDns: settings.value.ethDhcp ? [] : [required, ip],
      authLogin: settings.value.authMode ? [required, maxLen(12)] : [],
      authPass: settings.value.authMode ? [required, maxLen(12)] : [],
      reAuthPassword: settings.value.authMode ? [required, maxLen(12), sameAs('authPass')] : [],
      discoveryInterval: settings.value.discovery ? [required, minVal(5), maxVal(255)] : [],
      discoveryPort: settings.value.discovery ? [required, minVal(1000), maxVal(65535)] : [],
    }) as TValidationSchema,
});

const [wifiSsid, wifiSsidProps] = defineField<string>('wifiSsid');
const [wifiPass, wifiPassProps] = defineField<string>('wifiPass');
const [rePassword, rePasswordProps] = defineField<string>('rePassword');
const [wifiIp, wifiIpProps] = defineField<string>('wifiIp');
const [wifiSubnet, wifiSubnetProps] = defineField<string>('wifiSubnet');
const [wifiGateway, wifiGatewayProps] = defineField<string>('wifiGateway');
const [wifiDns, wifiDnsProps] = defineField<string>('wifiDns');
const [ethIp, ethIpProps] = defineField<string>('ethIp');
const [ethSubnet, ethSubnetProps] = defineField<string>('ethSubnet');
const [ethGateway, ethGatewayProps] = defineField<string>('ethGateway');
const [ethDns, ethDnsProps] = defineField<string>('ethDns');
const [authLogin, authLoginProps] = defineField<string>('authLogin');
const [authPass, authPassProps] = defineField<string>('authPass');
const [reAuthPassword, reAuthPasswordProps] = defineField<string>('reAuthPassword');
const [discoveryInterval, discoveryIntervalProps] = defineField<string>('discoveryInterval');
const [discoveryPort, discoveryPortProps] = defineField<string>('discoveryPort');

watch(
  () => settings.value,
  () => {
    wifiSsid.value = settings.value.wifiSsid;
    if (settings.value.wifiPass) wifiPass.value = settings.value.wifiPass;
    if (settings.value.wifiPass) rePassword.value = settings.value.wifiPass;
    wifiIp.value = arrToStr(settings.value.wifiIp);
    wifiSubnet.value = arrToStr(settings.value.wifiSubnet);
    wifiGateway.value = arrToStr(settings.value.wifiGateway);
    wifiDns.value = arrToStr(settings.value.wifiDns);
    ethIp.value = arrToStr(settings.value.ethIp);
    ethSubnet.value = arrToStr(settings.value.ethSubnet);
    ethGateway.value = arrToStr(settings.value.ethGateway);
    ethDns.value = arrToStr(settings.value.ethDns);
    authLogin.value = settings.value.authLogin;
    authPass.value = settings.value.authPass;
    reAuthPassword.value = settings.value.authPass;
    discoveryPort.value = `${settings.value.discoveryPort}`;
    discoveryInterval.value = `${settings.value.discoveryInterval}`;
  },
  { immediate: true }
);

const onSubmit = handleSubmit((values) => {
  settings.value.wifiSsid = values.wifiSsid as string;
  settings.value.wifiPass = values.wifiPass as string;
  settings.value.wifiIp = strToArr(values.wifiIp as string);
  settings.value.wifiSubnet = strToArr(values.wifiSubnet as string);
  settings.value.wifiGateway = strToArr(values.wifiGateway as string);
  settings.value.wifiDns = strToArr(values.wifiDns as string);
  settings.value.ethIp = strToArr(values.ethIp as string || ethIp.value);
  settings.value.ethSubnet = strToArr(values.ethSubnet as string || ethSubnet.value);
  settings.value.ethGateway = strToArr(values.ethGateway as string || ethGateway.value);
  settings.value.ethDns = strToArr(values.ethDns as string || ethDns.value);
  settings.value.authLogin = values.authLogin as string;
  settings.value.authPass = values.authPass as string;
  settings.value.discoveryPort = Number(values.discoveryPort);
  settings.value.discoveryInterval = Number(values.discoveryInterval);
  onSave();
});

const listWiFi: IListItem<number>[] = [
  { name: $t('off'), value: 0 },
  { name: $t('sta'), value: 1 },
  { name: $t('ap'), value: 2 },
];

const getValueListWiFi = (mode: number) => listWiFi.find((i) => i.value === mode)?.name ?? '';

const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.authMode));
const isEthDhcp = computed(() => Boolean(settings.value.ethDhcp));
const ethLink = computed(() => Boolean(main.value.info.ethernet));
const ethSpeed = computed(() => (main.value.info.ethSpeed ? `${main.value.info.ethSpeed} Mbps` : '—'));
const ethCurrentIp = computed(() => arrToStr(main.value.info.ethIp as number[]));

const onReboot = () => {
  onSend(KEY.REBOOT);
};

const onSave = () => {
  onSend(KEY.SETTINGS, settings.value);
  onSureReboot();
};

const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => {
    onSave();
    onReboot();
  });
};

const onSureReboot = () => onDialog({ value: true, message: $t('dialog.doRestart'), callback: onReboot });
const onSureReset = () => onDialog({ value: true, message: $t('dialog.doReset'), callback: onReset });

const onMenu = () => {
  onSubmit();
};

const onSelectSsid = ({ ssid = '' }: Partial<IScan>) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  wifiSsid.value = ssid;
  wifiPass.value = '';
  rePassword.value = '';
  const input: HTMLInputElement | null = document.querySelector('#wifiPass input');
  if (input) input.focus();
  onClose();
};

const onClose = () => (isDialog.value = false);

const onScan = () => {
  isDialog.value = true;
  scanList.value = [];
  onSend(KEY.SCAN);
};

const onChange = (value: number) => (settings.value.wifiMode = value);
const onSureOffWifi = ({ value }: IListItem<number>) => {
  const v = value as number;
  return !v ? onDialog({ value: true, message: $t('dialog.doReset'), callback: onChange.bind(this, v) }) : onChange(v);
};

const onFlash = async (body: FormData) => {
  const { data } = await useFetch('/update', { body }).post().json();
  if (data.value?.state) onDialog({ value: true, title: 'Done', message: 'Reboot...' });
};

const onUpdate = (name: string, files: FormData) => {
  if (!files) return;
  onDialog({
    value: true,
    message: $t('dialog.doUpdate', { name }),
    callback: name === 'firmware' ? onFlash.bind(null, files) : onFlash.bind(null, files),
  });
};
</script>

<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
    <VCard class="xl:col-span-2 2xl:col-span-3" hover>
      <div class="grid grid-cols-1 xl:grid-cols-2 gap-x-4">
        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-4 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-dark-30 dark:text-white/90">{{ $t('wifi') }}</h3>
          </div>

          <div class="mb-6">
            <v-select :model-value="getValueListWiFi(settings.wifiMode)" :label="$t('mode')" :items="listWiFi" @change="onSureOffWifi" />
          </div>

          <v-text-field v-model="wifiSsid" v-bind="wifiSsidProps" :label="$t('ssid')" :disabled="isWifi" @on-icon="onScan">
            <template #icon>
              <VIcon name="search" class="size-4"></VIcon>
            </template>
          </v-text-field>

          <v-text-field id="wifiPass" v-model="wifiPass" v-bind="wifiPassProps" :label="$t('pass')" :disabled="isWifi" :type="isPass ? 'text' : 'password'" @on-icon="isPass = !isPass">
            <template #icon>
              <VIcon name="eye" v-if="isPass" class="size-5"></VIcon>
              <VIcon name="eye-off" v-else class="size-5"></VIcon>
            </template>
          </v-text-field>

          <v-text-field v-model="rePassword" v-bind="rePasswordProps" :label="$t('passRe')" :disabled="isWifi" :type="isPass ? 'text' : 'password'" @on-icon="isPass = !isPass">
            <template #icon>
              <VIcon name="eye" v-if="isPass" class="size-5"></VIcon>
              <VIcon name="eye-off" v-else class="size-5"></VIcon>
            </template>
          </v-text-field>
        </div>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-4 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-dark-30 dark:text-white/90 mb-0">{{ $t('ipSett') }}</h3>

            <div>
              <VCheckbox :model-value="Boolean(settings.wifiDhcp)" @update:model-value="settings.wifiDhcp = $event ? 1 : 0">{{ $t('dhcp') }}</VCheckbox>
            </div>
          </div>

          <v-text-field v-model="wifiIp" v-bind="wifiIpProps" :label="$t('ip')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiSubnet" v-bind="wifiSubnetProps" :label="$t('subnet')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiGateway" v-bind="wifiGatewayProps" :label="$t('gateway')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiDns" v-bind="wifiDnsProps" :label="$t('dns')" :disabled="isWifiDHCP" />
        </div>
      </div>
    </VCard>

    <VCard :title="$t('eth.title')" hover>
      <template #header>
        <div class="col-span-full flex flex-wrap items-center gap-3">
          <VCheckbox :model-value="Boolean(settings.ethEnabled)" @update:model-value="settings.ethEnabled = $event ? 1 : 0">{{ $t('eth.enabled') }}</VCheckbox>
          <VCheckbox :model-value="Boolean(settings.ethDhcp)" :disabled="!settings.ethEnabled" @update:model-value="settings.ethDhcp = $event ? 1 : 0">{{ $t('dhcp') }}</VCheckbox>
          <span class="text-xs font-semibold" :class="ethLink ? 'text-emerald-500' : 'text-amber-500'">
            {{ ethLink ? $t('eth.linkUp') : $t('eth.linkDown') }}
          </span>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="ethIp" v-bind="ethIpProps" :label="$t('ip')" :disabled="isEthDhcp || !settings.ethEnabled" />
        <v-text-field v-model="ethSubnet" v-bind="ethSubnetProps" :label="$t('subnet')" :disabled="isEthDhcp || !settings.ethEnabled" />
        <v-text-field v-model="ethGateway" v-bind="ethGatewayProps" :label="$t('gateway')" :disabled="isEthDhcp || !settings.ethEnabled" />
        <v-text-field v-model="ethDns" v-bind="ethDnsProps" :label="$t('dns')" :disabled="isEthDhcp || !settings.ethEnabled" />
      </div>

      <div class="mt-4 grid grid-cols-2 gap-3 border-t border-gray-light-10 pt-3 text-sm dark:border-white/10">
        <div>
          <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('eth.speed') }}</div>
          <div class="mt-1 font-mono">{{ ethSpeed }}</div>
        </div>
        <div>
          <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('eth.currentIp') }}</div>
          <div class="mt-1 font-mono">{{ ethCurrentIp }}</div>
        </div>
      </div>
    </VCard>

    <VCard :title="$t('security')" hover>
      <template #header>
        <div class="col-span-full">
          <VCheckbox :model-value="Boolean(settings.authMode)" @update:model-value="settings.authMode = $event ? 1 : 0">{{ $t('auth') }}</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="authLogin" v-bind="authLoginProps" :label="$t('login')" :disabled="isAuth" />

        <v-text-field v-model="authPass" v-bind="authPassProps" :label="$t('pass')" class="col-end-2" :type="isAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="isAuthPass = !isAuthPass">
          <template #icon>
            <VIcon name="eye" v-if="isAuthPass" class="size-5"></VIcon>
            <VIcon name="eye-off" v-else class="size-5"></VIcon>
          </template>
        </v-text-field>

        <v-text-field v-model="reAuthPassword" v-bind="reAuthPasswordProps" :label="$t('pass')" :type="isAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="isAuthPass = !isAuthPass">
          <template #icon>
            <VIcon name="eye" v-if="isAuthPass" class="size-5"></VIcon>
            <VIcon name="eye-off" v-else class="size-5"></VIcon>
          </template>
        </v-text-field>
      </div>
    </VCard>

    <VCard :title="$t('update')" hover>
      <BlockUpdate v-slot="{ file }" @update="onUpdate">
        <span>{{ file || $t('selectFile') }}</span>
      </BlockUpdate>
    </VCard>

    <VCard :title="$t('system')" hover>
      <template #header>
        <button class="text-gray-main cursor-pointer" @click="dialogInfo = true">
          <VIcon name="info" class="size-5" />
        </button>
      </template>

      <BlockService :locale="getLocale()" :locales="listLocale" @locale="onLocale" @reset="onSureReset" @reboot="onSureReboot" />
    </VCard>

    <VCard :title="`${$t('discovery')} (UDP)`" hover>
      <template #header>
        <div class="col-span-full">
          <VCheckbox :model-value="Boolean(settings.discovery)" @update:model-value="settings.discovery = $event ? 1 : 0">{{ $t('discovery') }}</VCheckbox>
        </div>
      </template>

      <v-text-field v-model="discoveryPort" v-bind="discoveryPortProps" :label="$t('port')" :disabled="!settings.discovery" />

      <v-text-field v-model="discoveryInterval" v-bind="discoveryIntervalProps" :label="$t('interval')" :disabled="!settings.discovery" />
    </VCard>

    <VCard :title="$t('diag.title')" class="xl:col-span-2 2xl:col-span-2" hover>
      <BlockDiagnostics />
    </VCard>

    <teleport to="#device-slot">
      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <VIcon name="more" class="size-5 rotate-90"></VIcon>
          </v-button>
        </template>
      </v-select>
    </teleport>

    <v-dialog v-if="isDialog" :title="$t('scan')" @close="onClose">
      <BlockScan :items="scanList" @select="onSelectSsid" @scan="onScan" />
    </v-dialog>
  </div>
</template>
