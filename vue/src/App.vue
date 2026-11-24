<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <PageBackdrop />

    <AppOverlay v-if="!isConnect">
      <div class="text-2xl font-bold mb-4">{{ $t('disconn') }}</div>

      <div class="flex justify-center">
        <v-loader class="text-cyan-main"></v-loader>
      </div>
    </AppOverlay>

    <div class="relative z-10 flex h-screen overflow-hidden">
      <AppAside v-if="!isIframe" :isSidebar="isSidebar" @sidebar="onSidebar">
        <h3 class="mb-4 text-xs h-5 uppercase flex items-center gap-2 justify-between" :class="isSidebar ? 'lg:hidden' : ''">
          <span v-if="nameDevice" class="text-gray-main text-nowrap">{{ nameDevice }}</span>
        </h3>

        <AppNav :menu="getMenu" :isSidebar="isSidebar" :fullPath="fullPath" @sidebar="onSidebar">
          <template #icon="{ item }">
            <VIcon :name="item.icon || 'home'" class="size-5" />
          </template>
        </AppNav>

        <div class="flex-auto"></div>

        <BlockStatus
          v-bind="info"
          :pkg="pkg"
          class="mb-4 w-full rounded-md border border-gray-light-10 dark:border-gray-dark-30 bg-gray-light-30 dark:bg-white/[0.03] px-4 py-4 text-center"
          :class="isSidebar ? 'lg:hidden' : ''"
        >
          <template #name="{ item }">
            {{ $t(item.name) }}
          </template>
          <template #default="{ repo, home }">
            <div class="grid grid-cols-2 gap-2 mt-4">
              <v-button :href="repo" target="_blank" color="gray" variant="soft">Github</v-button>

              <v-button v-if="home" :href="home" target="_blank" color="blue" variant="outline">{{ $t('homepage') }}</v-button>
            </div>
          </template>
        </BlockStatus>
      </AppAside>

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden scrollbar mobile-hide-scrollbar">
        <AppHeader v-if="!isIframe" :change-theme="changeTheme" :notifications="notifications" @sidebar="isSidebar = !isSidebar" @notif="isSidebarRight = !isSidebarRight"></AppHeader>

        <main :class="isIframe ? 'no-scrollbar' : 'py-6 flex-auto'">
          <div :class="isIframe ? '' : 'container mx-auto h-full'">
            <router-view />
          </div>
        </main>

        <AppProgress v-slot="{ isDone }" v-bind="progress" :timeout="2000" class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" @close="webSocketStore.CLEAR_PROGRESS">
          <div class="flex-auto">{{ $t('prog') }}</div>

          <span v-if="isDone" class="text-green-600">{{ $t('done') }}</span>
        </AppProgress>

        <AppNotification :value="isSidebarRight" :notifications="notifications" @close="isSidebarRight = false" @remove="appStore.removeNotification" @read="appStore.readNotification">
          <template #header="{ isNew }">
            <div class="flex items-center justify-between py-4 sticky top-0 z-20 px-2">
              <h5>{{ $t('noti') }}</h5>

              <VButton size="sm" color="gray" :disabled="!isNew" @click="appStore.readAllNotification">{{ $t('mark') }}</VButton>
            </div>
          </template>
          <template #empty>
            <p class="text-center py-10">{{ $t('empty') }}</p>
          </template>
        </AppNotification>
      </div>
    </div>

    <v-dialog v-if="dialog.value" v-slot="{ close }" :title="dialog?.title || $t('attention')" @close="dialog = {}">
      <div v-html="dialog.message" />

      <div v-if="dialog.callback" class="flex gap-3 mt-4 justify-end">
        <v-button class="min-w-[100px]" color="blue" variant="soft" @click="(dialog.callback(), close())">{{ $t('yes') }}</v-button>
        <v-button class="min-w-[100px]" color="red" variant="soft" @click="close()">{{ $t('no') }}</v-button>
      </div>
    </v-dialog>

    <v-dialog v-if="dialogInfo" size="md" title="Information" @close="dialogInfo = false">
      <BlockInfo v-bind="main.info" :pkg="pkg" class="w-full">
        <template #links="{ links }">
          {{ $t('links') }}:
          <div class="flex flex-wrap gap-2 text-gray-dark-30 dark:text-gray-main">
            <a v-for="link of links" :key="link.name" :href="link.href" target="_blank" class="me-4">{{ $t(link.name) }}</a>
          </div>
        </template>
      </BlockInfo>
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import { useRoute } from 'vue-router';

import { localGet } from 'vuesp-components/helpers';

import { useWebSocketStore } from '@/stores/WebSocketStore';
import { useAppStore } from '@/stores/AppStore';

import { useFrame } from '@/composables/useFrame';
import { useSocket } from '@/composables/useSocket';
import { useStore } from '@/composables/useStore';
import { useLocale } from '@/composables/useLocale';

import { AppHeader, AppNotification, AppOverlay, AppAside, AppNav, AppProgress, BlockInfo, BlockStatus, VIcon } from 'vuesp-components/components';
import PageBackdrop from '@/components/ui/PageBackdrop.vue';

const { $t, setLocale } = useLocale();

setLocale(localGet('locale') || navigator?.language);

const { isIframe } = useFrame();
const { isConnect } = useSocket();
const { pkg, menu, dialog, dialogInfo, main, notifications, progress, changeTheme } = useStore();

const webSocketStore = useWebSocketStore();
const appStore = useAppStore();

const isSidebar = ref(false);
const isSidebarRight = ref(false);

const route = useRoute();

const getMenu = computed(() => menu.value.map((i) => ({ ...i, name: $t(i.name) })));
const info = computed(() => main.value.info);
const nameDevice = computed(() => info.value?.name || '');

const fullPath = computed(() => route.fullPath);
const onSidebar = (value: boolean) => (isSidebar.value = value);
</script>
