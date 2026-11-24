<script setup lang="ts">
defineProps<{ title?: string }>();
</script>

<template>
  <div class="relative mb-6 flex items-center justify-between gap-4 overflow-hidden rounded-xl border border-gray-light-10/70 px-4 py-4 dark:border-white/5">
    <h1 class="min-w-0 truncate"><slot>{{ title }}</slot></h1>

    <div v-if="$slots.aside" class="shrink-0">
      <slot name="aside" />
    </div>
  </div>
</template>
