import type { IStateMain } from '@/types';
import { computed, ref, type Ref } from 'vue';
import type { IMessage } from '@/types';

export const useSlots = (main: Ref<IStateMain, IStateMain>) => {
  const currentSlot = ref<IMessage | null>(null);
  const slotInfo = computed(() => {
    return Object.values(main.value.slots).reduce(
      (acc, i: IMessage) => {
        if (i.active || i.handler) {
          acc.used = acc.used + 1;
        }
        acc.total = acc.total + 1;
        return acc;
      },
      { total: 0, used: 0 }
    );
  });

  const onSlot = (slot: IMessage) => {
    currentSlot.value = slot;
  };
  const getColorSlot = (slot: IMessage) => {
    if (slot.active) return 'bg-violet-500';
    if (slot.handler) return 'bg-blue-500';
    return 'bg-gray-light-10 dark:bg-gray-dark-25 ';
  };
  return {
    slotInfo,
    currentSlot,
    onSlot,
    getColorSlot,
  };
};
