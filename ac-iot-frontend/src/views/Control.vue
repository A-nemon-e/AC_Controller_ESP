<template>
  <div class="control-page">
    <!-- 设备选择器 -->
    <van-dropdown-menu v-if="devices.length > 0">
      <van-dropdown-item
        v-model="selectedDeviceId"
        :options="deviceOptions"
        @change="onDeviceChange"
      />
    </van-dropdown-menu>

    <div v-if="!currentDevice" class="empty-state">
      <van-empty description="暂无设备">
        <van-button type="primary" @click="$router.push('/settings')">
          添加设备
        </van-button>
      </van-empty>
    </div>

    <template v-else>
      <!-- 温湿度显示 -->
      <van-cell-group class="status-card" inset>
        <div class="temp-display">
          <div class="temp-value">{{ currentState?.temp || '--' }}°C</div>
          <div class="humidity">💧 湿度: {{ currentState?.hum || '--' }}%</div>
          <div v-if="config?.enableCurrent" class="current">
            ⚡ 电流: {{ currentState?.current?.toFixed(1) || '0.0' }}A
          </div>
        </div>
      </van-cell-group>

      <!-- 当前状态 -->
      <van-cell-group inset title="当前状态">
        <van-cell title="状态">
          <template #value>
            <van-tag :type="currentState?.power ? 'success' : 'default'">
              {{ currentState?.power ? '已开机' : '已关机' }}
            </van-tag>
          </template>
        </van-cell>
        <van-cell title="模式" :value="modeText" />
        <van-cell title="设定温度" :value="`${currentState?.setTemp || '--'}°C`" />
        <van-cell title="风速" :value="fanText" />
      </van-cell-group>

      <!-- 遥控器控制面板 -->
      <van-cell-group inset title="遥控器控制">
        <!-- 开关按钮 -->
        <div class="power-button">
          <van-button
            :type="command.power ? 'danger' : 'default'"
            size="large"
            block
            @click="command.power = !command.power"
          >
            {{ command.power ? '🔴 关机' : '⚪ 开机' }}
          </van-button>
        </div>

        <!-- 模式选择 -->
        <van-cell title="模式选择" />
        <div class="mode-selector">
          <van-grid :column-num="4" :border="false">
            <van-grid-item
              v-for="mode in modes"
              :key="mode.value"
              :text="mode.label"
              @click="command.mode = mode.value"
              :class="{ 'mode-active': command.mode === mode.value }"
            >
              <template #icon>
                <span class="mode-icon">{{ mode.icon }}</span>
              </template>
            </van-grid-item>
          </van-grid>
        </div>

        <!-- 温度调节 -->
        <van-cell title="温度调节" />
        <div class="temp-control">
          <van-button icon="minus" @click="decreaseTemp" />
          <span class="temp-value">{{ command.setTemp }}°C</span>
          <van-button icon="plus" @click="increaseTemp" />
        </div>
        <div class="slider-wrapper">
          <van-slider
            v-model="command.setTemp"
            :min="16"
            :max="30"
            :step="1"
            active-color="#1989fa"
          />
        </div>

        <!-- 风速 -->
        <van-cell title="风速" />
        <van-radio-group v-model="command.fan" direction="horizontal" class="fan-group">
          <van-radio name="auto">自动</van-radio>
          <van-radio name="low">低</van-radio>
          <van-radio name="mid">中</van-radio>
          <van-radio name="high">高</van-radio>
        </van-radio-group>

        <!-- 摆风 -->
        <van-cell title="摆风" />
        <div class="swing-controls">
          <van-checkbox v-model="command.swingVertical">↕️ 上下摆风</van-checkbox>
          <van-checkbox v-model="command.swingHorizontal">↔️ 左右摆风</van-checkbox>
        </div>

        <!-- 应用按钮 -->
        <div class="apply-button">
          <van-button
            type="primary"
            size="large"
            block
            :loading="sending"
            @click="applyCommand"
          >
            应用设置
          </van-button>
        </div>
      </van-cell-group>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { showToast, showLoadingToast, closeToast } from 'vant'
import { useDevicesStore } from '@/stores/devices'
import { devicesApi } from '@/api/devices'
import type { DeviceState } from '@/types/device'

const devicesStore = useDevicesStore()
const sending = ref(false)

const devices = computed(() => devicesStore.devices)
const currentDevice = computed(() => devicesStore.currentDevice)
const currentState = computed(() => currentDevice.value?.lastState)
const config = computed(() => currentDevice.value?.config)

const selectedDeviceId = ref<number>(currentDevice.value?.id || 0)

const deviceOptions = computed(() =>
  devices.value.map((device) => ({
    text: device.name,
    value: device.id,
  }))
)

const command = ref<Partial<DeviceState>>({
  power: false,
  mode: 'cool',
  setTemp: 26,
  fan: 'auto',
  swingVertical: false,
  swingHorizontal: false,
})

const modes = [
  { value: 'cool', label: '制冷', icon: '❄️' },
  { value: 'heat', label: '制热', icon: '♨️' },
  { value: 'fan', label: '送风', icon: '💨' },
  { value: 'dry', label: '除湿', icon: '💧' },
]

const modeText = computed(() => {
  const mode = modes.find((m) => m.value === currentState.value?.mode)
  return mode ? mode.label : '--'
})

const fanText = computed(() => {
  const fanMap = { auto: '自动', low: '低', mid: '中', high: '高' }
  return fanMap[currentState.value?.fan || 'auto']
})

const onDeviceChange = (value: number) => {
  const device = devices.value.find((d) => d.id === value)
  if (device) {
    devicesStore.setCurrentDevice(device)
  }
}

const decreaseTemp = () => {
  if (command.value.setTemp && command.value.setTemp > 16) {
    command.value.setTemp--
  }
}

const increaseTemp = () => {
  if (command.value.setTemp && command.value.setTemp < 30) {
    command.value.setTemp++
  }
}

const applyCommand = async () => {
  if (!currentDevice.value) return

  sending.value = true
  showLoadingToast({ message: '发送中...', forbidClick: true })

  try {
    await devicesApi.sendCommand(currentDevice.value.id, command.value)
    closeToast()
    showToast({ message: '命令已发送', icon: 'success' })
  } catch (error) {
    closeToast()
    showToast({ message: '发送失败', icon: 'fail' })
    console.error('Failed to send command:', error)
  } finally {
    sending.value = false
  }
}

// ✅ 新增：轮询刷新配置
const POLL_INTERVAL = 3000 // 3秒刷新一次
let pollTimer: number | null = null

// ✅ 新增：启动轮询
const startPolling = () => {
  if (pollTimer) return
  pollTimer = setInterval(async () => {
    if (currentDevice.value?.id) {
      await devicesStore.fetchDeviceStatus(currentDevice.value.id)
    }
  }, POLL_INTERVAL) as unknown as number
}

// ✅ 新增：停止轮询
const stopPolling = () => {
  if (pollTimer) {
    clearInterval(pollTimer)
    pollTimer = null
  }
}

// 同步当前状态到命令
watch(
  currentState,
  (state) => {
    if (state) {
      command.value = {
        power: state.power,
        mode: state.mode,
        setTemp: state.setTemp,
        fan: state.fan,
        swingVertical: state.swingVertical ?? false,
        swingHorizontal: state.swingHorizontal ?? false,
      }
    }
  },
  { immediate: true, deep: true }
)

// 同步设备选择
watch(
  currentDevice,
  (device) => {
    if (device) {
      selectedDeviceId.value = device.id
    }
  },
  { immediate: true }
)

// ✅ 新增：组件挂载时启动轮询
onMounted(() => {
  startPolling()
})

// ✅ 新增：组件卸载时停止轮询
onUnmounted(() => {
  stopPolling()
})
</script>

<style scoped>
.control-page {
  padding-bottom: 20px;
}

.empty-state {
  padding: 60px 20px;
}

.status-card {
  margin-top: 16px;
  margin-bottom: 16px;
}

.temp-display {
  text-align: center;
  padding: 32px 16px;
}

.temp-value {
  font-size: 56px;
  font-weight: bold;
  color: #1989fa;
  line-height: 1.2;
}

.humidity,
.current {
  margin-top: 12px;
  font-size: 16px;
  color: #969799;
}

.power-button,
.apply-button {
  margin: 16px;
}

.mode-selector {
  padding: 8px 16px;
}

.mode-icon {
  font-size: 28px;
}

.mode-active {
  background: #e8f4ff;
  border-radius: 8px;
}

.temp-control {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 24px;
  margin: 16px 0;
}

.temp-control .temp-value {
  font-size: 32px;
  font-weight: bold;
  min-width: 100px;
  text-align: center;
  color: #323233;
}

.slider-wrapper {
  padding: 0 16px 16px;
}

.fan-group {
  padding: 16px;
  display: flex;
  gap: 16px;
}

.swing-controls {
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
</style>
