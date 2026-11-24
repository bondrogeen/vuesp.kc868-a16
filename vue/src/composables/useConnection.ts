import { onMounted, onUnmounted } from 'vue';
import { event } from '@/assets/js/';
import Queue from '@/assets/js/queue';

import { useStore } from '@/composables/useStore';

import type { TMessage, TSend } from '@/types';
import type { TKey } from '@/generated/protocol';

export const useConnection = (init?: (send: TSend) => void, messages?: (message: TMessage) => void) => {
  const store = useStore();

  const queue = new Queue<{ key: TKey | number; object?: object }>(100, ({ key, object }) => {
    store.wsSend(key, object);
  });

  const onSend = (key: TKey | number, object?: object) => {
    queue.add({ key, object });
  };

  const onInit = () => {
    if (!store.isInit.value && init) init(onSend);
  };

  const onMessages = (message: TMessage) => {
    messages?.(message);
  };

  event.on('init', onInit);
  if (messages) event.on('messages', onMessages);

  onMounted(() => {
    if (store.isInit.value && init) init(onSend);
  });

  onUnmounted(() => {
    event.off('init', onInit);
    if (messages) event.off('messages', onMessages);
  });

  return { ...store, onSend };
};
