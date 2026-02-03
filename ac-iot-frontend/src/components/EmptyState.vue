<template>
  <van-empty
    :image="image"
    :description="description"
  >
    <slot></slot>
  </van-empty>
</template>

<script setup lang="ts">
defineProps<{
  image?: string
  description?: string
}>()
</script>
