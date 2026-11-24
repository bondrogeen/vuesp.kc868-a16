<script setup lang="ts">
import { VIcon } from 'vuesp-components/components';

import { localIcons } from '@/icons';

const { name } = defineProps<{ name: string }>();
</script>

<template>
	<VIcon :name="name" :icons="localIcons" class="size-full" />
</template>
