declare module '*.vue' {
  import { defineComponent } from 'vue';
  const component: ReturnType<typeof defineComponent>;
  export default component;
}

import type { II18nInstance, TI18nGet } from './i18n';

declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $i18n: II18nInstance;
    $t: TI18nGet;
  }
}

export {};
