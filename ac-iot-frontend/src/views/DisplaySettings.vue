<template>
  <div class="display-settings-page">
    <!-- LED预览区域 -->
    <van-cell-group inset title="实时预览" class="preview-section">
      <div class="led-preview-container">
        <div class="led-matrix" :style="matrixStyle">
          <div
            v-for="pixel in pixels"
            :key="pixel.id"
            class="led-pixel"
            :style="pixel.style"
          />
        </div>
        <div class="preview-info">
          <span class="card-name">{{ displayStore.currentCard?.name }}</span>
          <van-tag type="primary" v-if="displayStore.settings.autoSwitch">自动切换</van-tag>
        </div>
      </div>
    </van-cell-group>

    <!-- 卡片列表 -->
    <van-cell-group inset title="卡片列表" class="mt-12">
      <van-swipe-cell v-for="card in displayStore.cards" :key="card.id">
        <van-cell
          :title="card.name"
          :label="`${card.duration}秒 | ${card.foregrounds.length}个前景`"
        >
          <template #right-icon>
            <van-switch v-model="card.enabled" size="20" @change="toggleCard(card.id)" />
          </template>
        </van-cell>
        <template #right>
          <van-button
            square
            type="primary"
            text="编辑"
            @click="editCard(card)"
          />
          <van-button
            square
            type="danger"
            text="删除"
            @click="deleteCard(card.id)"
          />
        </template>
      </van-swipe-cell>
      
      <van-cell title="添加卡片" is-link @click="addCard">
        <template #icon>
          <van-icon name="plus" class="add-icon" />
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 全局设置 -->
    <van-cell-group inset title="全局设置" class="mt-12">
      <van-cell title="亮度" center>
        <template #right-icon>
          <van-slider
            v-model="displayStore.settings.brightness"
            :min="10"
            :max="100"
            :step="5"
            style="width: 150px"
            @change="saveSettings"
          />
        </template>
      </van-cell>
      
      <van-cell title="自动切换" center>
        <template #right-icon>
          <van-switch
            v-model="displayStore.settings.autoSwitch"
            @change="saveSettings"
          />
        </template>
      </van-cell>
      
      <van-cell
        v-if="displayStore.settings.autoSwitch"
        title="切换间隔"
        :value="`${displayStore.settings.switchInterval}秒`"
        is-link
        @click="showIntervalPicker = true"
      />
      
      <van-cell
        title="转场效果"
        :value="transitionLabel"
        is-link
        @click="showTransitionPicker = true"
      />
    </van-cell-group>

    <!-- 夜间模式 -->
    <van-cell-group inset title="夜间模式" class="mt-12">
      <van-cell title="启用夜间模式" center>
        <template #right-icon>
          <van-switch
            v-model="displayStore.settings.nightMode"
            @change="saveSettings"
          />
        </template>
      </van-cell>
      
      <van-cell
        title="开始时间"
        :value="displayStore.settings.nightModeStart"
        is-link
        @click="showTimePicker = 'start'"
      />
      
      <van-cell
        title="结束时间"
        :value="displayStore.settings.nightModeEnd"
        is-link
        @click="showTimePicker = 'end'"
      />
      
      <van-cell title="夜间亮度" center>
        <template #right-icon>
          <van-slider
            v-model="displayStore.settings.nightModeBrightness"
            :min="5"
            :max="50"
            :step="5"
            style="width: 150px"
            @change="saveSettings"
          />
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 天气叠加 -->
    <van-cell-group inset title="天气叠加" class="mt-12">
      <van-cell
        title="显示天气"
        is-link
        :to="'/weather'"
      />
    </van-cell-group>

    <!-- 卡片编辑器弹窗 -->
    <van-popup
      v-model:show="showCardEditor"
      position="bottom"
      :style="{ height: '90%' }"
      round
    >
      <CardEditor
        v-if="editingCard"
        :card="editingCard"
        @save="onCardSave"
        @cancel="showCardEditor = false"
      />
    </van-popup>

    <!-- 选择器弹窗 -->
    <van-popup v-model:show="showIntervalPicker" position="bottom" round>
      <van-picker
        title="切换间隔"
        :columns="intervalColumns"
        @confirm="onIntervalConfirm"
        @cancel="showIntervalPicker = false"
      />
    </van-popup>

    <van-popup v-model:show="showTransitionPicker" position="bottom" round>
      <van-picker
        title="转场效果"
        :columns="transitionColumns"
        @confirm="onTransitionConfirm"
        @cancel="showTransitionPicker = false"
      />
    </van-popup>

    <van-popup :show="!!showTimePicker" position="bottom" round @update:show="(val) => { if(!val) showTimePicker = null }">
      <van-time-picker
        :model-value="timePickerValue"
        title="选择时间"
        @confirm="onTimeConfirm"
        @cancel="showTimePicker = null"
      />
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useDisplayStore } from '@/stores/display'
import { showToast, showConfirmDialog } from 'vant'
import CardEditor from '@/components/display/CardEditor.vue'
import type { CardConfig } from '@/types/display'

const displayStore = useDisplayStore()

// 弹窗控制
const showCardEditor = ref(false)
const showIntervalPicker = ref(false)
const showTransitionPicker = ref(false)
const showTimePicker = ref<'start' | 'end' | null>(null)
const editingCard = ref<CardConfig | null>(null)

// LED矩阵像素
const pixels = computed(() => {
  const list = []
  const width = 64
  const height = 32
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      list.push({
        id: `${x}-${y}`,
        style: {
          left: `${(x / width) * 100}%`,
          top: `${(y / height) * 100}%`,
          backgroundColor: Math.random() > 0.5 ? '#ff6b6b' : '#4ecdc4',
          opacity: 0.3 + Math.random() * 0.7,
        }
      })
    }
  }
  return list
})

const matrixStyle = computed(() => ({
  width: '100%',
  aspectRatio: '2/1',
  background: '#000',
}))

const transitionLabel = computed(() => {
  const labels: Record<string, string> = {
    'none': '无',
    'fade': '淡入淡出',
    'slide': '滑动',
    'zoom': '缩放',
    'flip': '翻转',
  }
  return labels[displayStore.settings.transition] || '淡入淡出'
})

const timePickerValue = computed(() => {
  if (showTimePicker.value === 'start') {
    return displayStore.settings.nightModeStart.split(':')
  }
  return displayStore.settings.nightModeEnd.split(':')
})

const intervalColumns = [
  { text: '5秒', value: 5 },
  { text: '10秒', value: 10 },
  { text: '15秒', value: 15 },
  { text: '20秒', value: 20 },
  { text: '30秒', value: 30 },
  { text: '60秒', value: 60 },
]

const transitionColumns = [
  { text: '无', value: 'none' },
  { text: '淡入淡出', value: 'fade' },
  { text: '滑动', value: 'slide' },
  { text: '缩放', value: 'zoom' },
  { text: '翻转', value: 'flip' },
]

// Actions
const toggleCard = (_id: string) => {
  displayStore.saveSettings()
}

const addCard = () => {
  const newCard: CardConfig = {
    id: '',
    name: '新卡片',
    enabled: true,
    duration: 10,
    background: {
      type: 'solid',
      brightness: 128,
      color: '#1989fa',
    },
    foregrounds: [],
  }
  editingCard.value = newCard
  showCardEditor.value = true
}

const editCard = (card: CardConfig) => {
  editingCard.value = { ...card }
  showCardEditor.value = true
}

const deleteCard = async (id: string) => {
  try {
    await showConfirmDialog({
      title: '确认删除',
      message: '确定要删除这个卡片吗？',
    })
    displayStore.deleteCard(id)
    showToast('删除成功')
  } catch {
    // 取消
  }
}

const onCardSave = (card: CardConfig) => {
  if (card.id) {
    displayStore.updateCard(card.id, card)
  } else {
    displayStore.addCard(card)
  }
  showCardEditor.value = false
  showToast('保存成功')
}

const onIntervalConfirm = ({ selectedOptions }: any) => {
  displayStore.updateSettings({ switchInterval: selectedOptions[0].value })
  showIntervalPicker.value = false
  saveSettings()
}

const onTransitionConfirm = ({ selectedOptions }: any) => {
  displayStore.updateSettings({ transition: selectedOptions[0].value })
  showTransitionPicker.value = false
  saveSettings()
}

const onTimeConfirm = ({ selectedValues }: any) => {
  const time = selectedValues.join(':')
  if (showTimePicker.value === 'start') {
    displayStore.updateSettings({ nightModeStart: time })
  } else {
    displayStore.updateSettings({ nightModeEnd: time })
  }
  showTimePicker.value = null
  saveSettings()
}

const saveSettings = () => {
  displayStore.saveSettings()
}

onMounted(() => {
  displayStore.fetchSettings()
})
</script>

<style scoped>
.display-settings-page {
  padding-bottom: 20px;
}

.preview-section {
  margin-bottom: 12px;
}

.led-preview-container {
  padding: 16px;
}

.led-matrix {
  position: relative;
  border-radius: 8px;
  overflow: hidden;
  margin-bottom: 12px;
}

.led-pixel {
  position: absolute;
  width: 3px;
  height: 3px;
  border-radius: 1px;
}

.preview-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.card-name {
  font-size: 14px;
  color: #323233;
}

.add-icon {
  color: #07c160;
  margin-right: 4px;
}

.mt-12 {
  margin-top: 12px;
}
</style>