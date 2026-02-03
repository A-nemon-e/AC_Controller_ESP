<template>
  <van-loading v-if="loading" class="loading" vertical>加载中...</van-loading>
</template>

<script setup lang="ts">
import { ref } from 'vue'

defineProps<{
  loading?: boolean
}>()
</script>

<style scoped>
.loading {
  padding: 40px 0;
  text-align: center;
}
</style>
