<template>
  <div class="control-page">
    <van-cell-group inset title="设备状态">
      <van-cell title="运行状态" :value="isRunning ? '运行中' : '已停止'">
        <template #right-icon>
          <van-switch v-model="isRunning" size="24" />
        </template>
      </van-cell>
      <van-cell title="当前温度" :value="`${currentTemp}°C`" />
      <van-cell title="设定温度" :value="`${targetTemp}°C`" />
    </van-cell-group>

    <van-cell-group inset title="温度控制" class="mt-12">
      <div class="temp-control">
        <van-button icon="minus" @click="decreaseTemp" />
        <span class="temp-display">{{ targetTemp }}°C</span>
        <van-button icon="plus" @click="increaseTemp" />
      </div>
    </van-cell-group>

    <van-cell-group inset title="模式选择" class="mt-12">
      <van-grid :column-num="4">
        <van-grid-item
          v-for="mode in modes"
          :key="mode.value"
          :icon="mode.icon"
          :text="mode.label"
          :class="{ active: currentMode === mode.value }"
          @click="currentMode = mode.value"
        />
      </van-grid>
    </van-cell-group>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'

const isRunning = ref(false)
const currentTemp = ref(25)
const targetTemp = ref(26)
const currentMode = ref('cool')

const modes = [
  { value: 'cool', label: '制冷', icon: 'snow-o' },
  { value: 'heat', label: '制热', icon: 'fire-o' },
  { value: 'dry', label: '除湿', icon: 'water-o' },
  { value: 'fan', label: '送风', icon: 'like-o' },
]

const increaseTemp = () => {
  if (targetTemp.value < 30) targetTemp.value++
}

const decreaseTemp = () => {
  if (targetTemp.value > 16) targetTemp.value--
}
</script>

<style scoped>
.control-page {
  padding-bottom: 20px;
}

.temp-control {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 24px;
  padding: 24px;
}

.temp-display {
  font-size: 48px;
  font-weight: bold;
  color: #323233;
}

:deep(.van-grid-item__content--active) {
  background: #e3f2fd;
}
</style>