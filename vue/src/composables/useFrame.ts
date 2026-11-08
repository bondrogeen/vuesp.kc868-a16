import { onMounted, ref, onUnmounted, watch, type WatchStopHandle } from 'vue';

import { useAppStore } from '@/stores/AppStore';
import { useLocale } from '@/composables/useLocale';
import { useRouter, useRoute } from 'vue-router';

export const useFrame = () => {
  const appStore = useAppStore();
  const { setLocale } = useLocale();

  const route = useRoute();
  const router = useRouter();

  let messageListener: ((event: MessageEvent) => void) | null = null;
  let stopRouteWatch: WatchStopHandle | null = null;

  const isIframe = ref(false);

  onMounted(async () => {
    appStore.init(route.query);

    if (window.self !== window.top) {
      isIframe.value = true;
      const html = document.querySelector('html');
      if (html) {
        html.classList.add('no-scrollbar');
      }

      messageListener = (event: MessageEvent) => {
        if (event?.data?.type === 'theme') {
          appStore.changeTheme(event.data.value);
        }
        if (event?.data?.type === 'locale' && typeof event.data.value === 'string') {
          setLocale(event.data.value);
        }
        if (event?.data?.type === 'route') {
          const path = event.data.data?.path;
          if (typeof path === 'string') void router.push(path);
        }
      };
      window.addEventListener('message', messageListener);

      stopRouteWatch = watch(
        () => route.fullPath,
        (path) => {
          window.parent.postMessage({ type: 'route', data: { path } }, '*');
        },
        { immediate: true },
      );

      window.parent.postMessage({ type: 'vuesp:ready' }, '*');
    }
  });

  onUnmounted(() => {
    if (messageListener) window.removeEventListener('message', messageListener);
    if (stopRouteWatch) stopRouteWatch();
  });

  return { isIframe };
};
