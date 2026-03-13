<template>
  <div class="card-editor">
    <van-nav-bar
      :title="isEdit ? '编辑卡片' : '新建卡片'"
      left-text="取消"
      right-text="保存"
      @click-left="handleCancel"
      @click-right="handleSave"
    />
    
    <van-cell-group inset class="mt-12">
      <van-field
        v-model="form.name"
        label="卡片名称"
        placeholder="请输入卡片名称"
        :rules="[{ required: true }]"
      />
      
      <van-cell title="背景类型" :value="form.background?.type || '无'" is-link />
      
      <van-cell title="启用" center>
        <template #right-icon>
          <van-switch v-model="form.enabled" />
        </template>
      </van-cell>
      
      <van-cell title="显示时长" :value="`${form.duration}秒`" is-link />
    </van-cell-group>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { CardConfig } from '@/types/display'

const props = defineProps<{
  card: CardConfig | null
}>()

const emit = defineEmits<{
  save: [card: CardConfig]
  cancel: []
}>()

const isEdit = computed(() => !!props.card?.id)

const form = ref<CardConfig>({
  id: props.card?.id || String(Date.now()),
  name: props.card?.name || '新卡片',
  enabled: props.card?.enabled ?? true,
  duration: props.card?.duration || 10,
  background: props.card?.background || null,
  foregrounds: props.card?.foregrounds || [],
})

const handleSave = () => {
  emit('save', form.value)
}

const handleCancel = () => {
  emit('cancel')
}
</script>

<style scoped>
.card-editor {
  height: 100%;
  background: #f7f8fa;
}

.mt-12 {
  margin-top: 12px;
}
</style>
