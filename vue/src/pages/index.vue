<script setup lang="ts">
import { computed } from 'vue';

import { VIcon, EnergyFlowBlock, deviceBlockStorage, energyFlowDefinition, useBlock, VCard } from 'vuesp-components/components';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';
import { KEY } from '@/utils/const';
import { CMD } from '@/generated/protocol';
import PageHeader from '@/components/ui/PageHeader.vue';

const { $t } = useLocale();

const { main } = useConnection((send) => {
  send(KEY.INFO);
  send(KEY.DEVICE);
  send(KEY.PORT, { gpio: 0, command: CMD.GET_ALL });
});

const block = useBlock(energyFlowDefinition, main, { storage: deviceBlockStorage });

const resources = computed(() => [
  { href: 'https://docs.vuesp.ru/', icon: 'book', title: $t('homePage.docs'), text: $t('homePage.docsText') },
  { href: 'https://docs.vuesp.ru/vuesp-components-examples', icon: 'grid', title: $t('homePage.components'), text: $t('homePage.componentsText') },
  { href: 'https://docs.vuesp.ru/web-ui', icon: 'code', title: $t('homePage.webUi'), text: $t('homePage.webUiText') },
  { href: 'https://github.com/bondrogeen/vuesp-components', icon: 'github', title: 'vuesp-components', text: $t('homePage.componentsSource') },
]);
</script>

<template>
  <div class="space-y-6 pb-10">
    <PageHeader :title="$t('home')" />

    <EnergyFlowBlock
      v-bind="block.data"
      :config="block.config"
      :context="block.context"
      @update:config="block.update"
    />

    <VCard :title="$t('homePage.docs')" hover>
      <p class="text-sm leading-relaxed text-gray-dark-25 dark:text-gray-main">{{ $t('homePage.docsIntro') }}</p>
      <div class="mt-4 grid gap-2 sm:grid-cols-2">
        <a
          v-for="resource of resources"
          :key="resource.href"
          :href="resource.href"
          target="_blank"
          rel="noreferrer"
          class="flex items-center gap-3 rounded-md bg-white/5 p-3 transition-colors hover:bg-white/10"
        >
          <VIcon name="book-open" v-if="resource.icon === 'book'" class="size-4 shrink-0 text-cyan-main" />
          <VIcon name="layout-grid" v-else-if="resource.icon === 'grid'" class="size-4 shrink-0 text-cyan-main" />
          <VIcon name="code" v-else-if="resource.icon === 'code'" class="size-4 shrink-0 text-cyan-main" />
          <VIcon name="github" v-else-if="resource.icon === 'github'" class="size-4 shrink-0 text-cyan-main" />
          <span class="min-w-0 flex-1">
            <span class="block text-xs font-medium">{{ resource.title }}</span>
            <span class="mt-0.5 block text-xs leading-snug text-gray-dark-25 dark:text-gray-main">{{ resource.text }}</span>
          </span>
          <VIcon name="external" class="size-3.5 shrink-0 text-gray-main" />
        </a>
      </div>
    </VCard>
  </div>
</template>
