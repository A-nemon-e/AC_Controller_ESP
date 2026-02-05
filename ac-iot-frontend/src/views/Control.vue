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

    <!-- 无设备状态 -->
    <div v-if="!currentDevice" class="empty-state">
      <van-empty description="暂无设备">
        <van-button type="primary" @click="$router.push('/settings')">
          添加设备
        </van-button>
      </van-empty>
    </div>

    <!-- 主要控制区域 -->
    <template v-else>
      <div class="main-container">
        
        <!-- 1. 顶部状态栏 (分左右屏) -->
        <div class="top-dashboard">
          <!-- 左侧：环境状态 -->
          <div class="status-panel">
            <div class="status-item">
              <div class="label">环境温度</div>
              <div class="value">{{ currentState?.temp || '--' }}<span class="unit">°C</span></div>
            </div>
            <div class="status-item">
              <div class="label">环境湿度</div>
              <div class="value">{{ currentState?.hum || '--' }}<span class="unit">%</span></div>
            </div>
          </div>

          <!-- 右侧：设定目标 + 发送 -->
          <div class="target-panel">
            <div class="label">设定温度</div>
            <div class="value target-temp">{{ command.setTemp }}<span class="unit">°C</span></div>
            <van-button 
              type="primary" 
              size="small" 
              class="send-btn" 
              :loading="sending" 
              @click="applyCommand"
            >
              提交/发送
            </van-button>
          </div>
        </div>

        <!-- 2. 中部遥控按键区 -->
        <div class="remote-pad">
          
          <!-- 温度加减行 -->
          <div class="pad-row temp-row">
            <div class="oval-btn" @click="handleInteraction(decreaseTemp)">
              <span class="btn-text">温度 -</span>
            </div>
            <div class="oval-btn" @click="handleInteraction(increaseTemp)">
              <span class="btn-text">温度 +</span>
            </div>
          </div>

          <!-- 电源大按钮 (居中) -->
          <div class="pad-row power-row">
            <div 
              class="power-circle" 
              :class="{ 'is-on': command.power }"
              @click="handleInteraction(togglePower)"
            >
              <div class="power-inner">
                <span class="icon">⏻</span>
                <span class="text">{{ command.power ? 'ON' : 'OFF' }}</span>
              </div>
            </div>
          </div>

          <!-- 风量/摆风行 -->
          <div class="pad-row func-row">
            <div class="oval-btn" @click="handleInteraction(cycleFan)">
              <span class="btn-label">风量</span>
              <span class="btn-value">{{ fanText }}</span>
            </div>
            <div class="oval-btn" @click="handleInteraction(toggleSwing)">
              <span class="btn-label">摆风</span>
              <span class="btn-value">{{ swingText }}</span>
            </div>
          </div>

        </div>

        <!-- 3. 底部模式选择行 -->
        <div class="mode-bar">
          <div 
            v-for="mode in modes"
            :key="mode.value"
            class="mode-item"
            :class="{ active: command.mode === mode.value }"
            @click="handleInteraction(() => command.mode = mode.value)"
          >
            <span class="mode-name">{{ mode.label }}</span>
          </div>
        </div>

        <!-- 4. Model 切换提示 (STRICTLY PRESERVED) -->
        <div class="model-tip" @click="openModelSwitcher">
          部分控制无效？点击此处切换 Model (当前: {{ brandSetup?.model || 1 }}) ➡️
        </div>

      </div>
    </template>

    <!-- Model 切换面板 (STRICTLY PRESERVED) -->
    <van-action-sheet v-model:show="showModelSheet" title="切换 Model">
      <div class="model-sheet-content">
        <div class="current-info">
          <div>当前协议: <van-tag type="primary">{{ brandSetup?.brand || '未设置' }}</van-tag></div>
          <div>当前 Model: <span class="model-id">{{ brandSetup?.model || 1 }}</span></div>
        </div>

        <div class="model-actions">
           <van-button icon="arrow-left" @click="changeModel(-1)" :disabled="switchingModel" />
           <van-button type="primary" @click="changeModel(1)" :loading="switchingModel">
             测试下一 Model (+1)
           </van-button>
        </div>
         
        <div class="model-input-row">
           <van-field v-model.number="customModelId" type="digit" label="指定ID" placeholder="输入ID" style="width: 150px" />
           <van-button size="small" type="success" @click="applyCustomModel">应用</van-button>
        </div>

        <div class="sheet-tip">
          提示: 不同 Model 对应同一品牌下的不同具体的红外编码格式。请逐个尝试直到空调响应。
        </div>
      </div>
    </van-action-sheet>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { showToast, showLoadingToast, closeToast } from 'vant'
import { useDevicesStore } from '@/stores/devices'
import { devicesApi } from '@/api/devices'
import type { DeviceState } from '@/types/device'

const router = useRouter()
const devicesStore = useDevicesStore()

const sending = ref(false)
const switchingModel = ref(false)
const showModelSheet = ref(false)
const customModelId = ref(1)

// --- Interaction Lock Logic ---
const lastInteractionTime = ref(0)
const INTERACTION_TIMEOUT = 5000 // 5 seconds lock

const handleInteraction = (fn: () => void) => {
  lastInteractionTime.value = Date.now()
  fn()
}

const devices = computed(() => devicesStore.devices)
const currentDevice = computed(() => devicesStore.currentDevice)
const currentState = computed(() => currentDevice.value?.lastState)
const config = computed(() => currentDevice.value?.config)

const brandSetup = computed(() => {
  if (!currentDevice.value?.brandConfig) return null
  try {
    const raw = currentDevice.value.brandConfig
    const parsed = typeof raw === 'string' ? JSON.parse(raw) : raw
    return {
        brand: parsed.brand || parsed.brandId,
        model: parsed.model
    }
  } catch (e) {
    console.error('Failed to parse brandConfig:', e)
    return null
  }
})

const selectedDeviceId = ref<number>(currentDevice.value?.id || 0)

const deviceOptions = computed(() =>
  devices.value.map((device) => ({
    text: device.name,
    value: device.id,
  }))
)

// Initial Command State
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
  { value: 'dry', label: '除湿', icon: '💧' },
  { value: 'fan', label: '送风', icon: '💨' },
]

const fanLevels = ['auto', 'low', 'mid', 'high']

// --- Computed Helpers ---
const fanText = computed(() => {
  const map: Record<string, string> = { auto: '自动', low: '低', mid: '中', high: '高' }
  return map[command.value.fan || 'auto']
})

const swingText = computed(() => {
  if (command.value.swingVertical) return '开启'
  return '关闭'
})

// --- Control Logic ---
const onDeviceChange = (value: number) => {
  const device = devices.value.find((d) => d.id === value)
  if (device) {
    devicesStore.setCurrentDevice(device)
  }
}

const togglePower = () => {
  command.value.power = !command.value.power
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

const cycleFan = () => {
  const current = command.value.fan || 'auto'
  const idx = fanLevels.indexOf(current)
  const nextIdx = (idx + 1) % fanLevels.length
  command.value.fan = fanLevels[nextIdx]
}

const toggleSwing = () => {
  // Simple toggle for vertical swing
  command.value.swingVertical = !command.value.swingVertical
}

const applyCommand = async () => {
  if (!currentDevice.value) return

  sending.value = true
  showLoadingToast({ message: '发送中...', forbidClick: true })

  try {
    await devicesApi.sendCommand(currentDevice.value.id, command.value)
    closeToast()
    showToast({ message: '命令已发送', icon: 'success' })
    // Reset interaction time to allow immediate sync if desired, 
    // or keep lock to prevent jitter. Let's keep existing lock logic natural.
  } catch (error) {
    closeToast()
    showToast({ message: '发送失败', icon: 'fail' })
  } finally {
    sending.value = false
  }
}

// --- Polling Logic ---
const POLL_INTERVAL = 3000
let pollTimer: number | null = null

const startPolling = () => {
  if (pollTimer) return
  pollTimer = setInterval(async () => {
    if (currentDevice.value?.id) {
      await devicesStore.fetchDeviceStatus(currentDevice.value.id)
    }
  }, POLL_INTERVAL) as unknown as number
}

const stopPolling = () => {
  if (pollTimer) {
    clearInterval(pollTimer)
    pollTimer = null
  }
}

// --- Watchers ---

// Sync Current State -> Command (With Anti-Conflict Lock)
watch(
  currentState,
  (state) => {
    if (state) {
      const now = Date.now()
      // ✅ Key Logic: If user interacted recently, DO NOT overwrite command
      if (now - lastInteractionTime.value < INTERACTION_TIMEOUT) {
        return
      }

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

watch(
  currentDevice,
  (device) => {
    if (device) {
      selectedDeviceId.value = device.id
    }
  },
  { immediate: true }
)

onMounted(() => {
  startPolling()
})

onUnmounted(() => {
  stopPolling()
})

// --- Model Switching Logic (Strictly Preserved) ---
const openModelSwitcher = () => {
  if (!brandSetup.value?.brand) {
    showToast('请先在设置页配置品牌')
    return
  }
  customModelId.value = brandSetup.value.model || 1
  showModelSheet.value = true
}

const changeModel = async (delta: number) => {
  if (!currentDevice.value || !brandSetup.value) return
  
  const newModel = (brandSetup.value.model || 1) + delta
  if (newModel < 1) return

  await doUpdateModel(newModel)
}

const applyCustomModel = async () => {
  if (!customModelId.value || customModelId.value < 1) return
  await doUpdateModel(customModelId.value)
}

const doUpdateModel = async (newModel: number) => {
  if (!currentDevice.value || !brandSetup.value) return
  
  switchingModel.value = true
  try {
    const brand = brandSetup.value.brand ||  brandSetup.value.brandId;
    await devicesApi.setBrand(currentDevice.value.id, brand, newModel)
    showToast({ message: `已切换至 Model ${newModel}`, icon: 'success' })
    
    await devicesStore.fetchDeviceStatus(currentDevice.value.id)
    await devicesApi.sendCommand(currentDevice.value.id, command.value)

  } catch (e) {
    showToast('切换失败')
  } finally {
    switchingModel.value = false
  }
}
</script>

<style scoped>
.control-page {
  padding-bottom: 20px;
  background-color: #f7f8fa;
  min-height: 100vh;
}

.empty-state {
  padding: 60px 20px;
}

.main-container {
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

/* --- Top Dashboard --- */
.top-dashboard {
  background: white;
  border-radius: 12px;
  display: flex;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0,0,0,0.05);
}

.status-panel, .target-panel {
  flex: 1;
  padding: 20px 16px;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.status-panel {
  border-right: 1px solid #f0f0f0;
  gap: 16px;
}

.status-item .label {
  font-size: 14px;
  color: #888;
  margin-bottom: 4px;
}

.status-item .value {
  font-size: 20px;
  font-weight: 600;
  color: #333;
}

.target-panel {
  align-items: center;
  gap: 10px;
  background: #fafafa;
}

.target-panel .label {
  font-size: 14px;
  color: #888;
}

.target-panel .target-temp {
  font-size: 36px;
  font-weight: bold;
  color: #1989fa;
  line-height: 1;
  margin-bottom: 5px;
}

.unit {
  font-size: 14px;
  margin-left: 2px;
}

.send-btn {
  width: 100%;
  border-radius: 8px;
  font-weight: bold;
}

/* --- Remote Pad --- */
.remote-pad {
  padding: 0 10px;
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.pad-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.power-row {
  justify-content: center;
}

/* Oval Buttons (Temp, Fan, Swing) */
.oval-btn {
  background: white;
  border: 2px solid #e8e8e8;
  border-radius: 999px; /* Pill shape */
  padding: 12px 20px;
  min-width: 100px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s;
  box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}

.oval-btn:active {
  background: #f0f0f0;
  transform: scale(0.98);
}

.btn-text {
  font-size: 16px;
  font-weight: 600;
  color: #333;
}

.btn-label {
  font-size: 12px;
  color: #888;
}

.btn-value {
  font-size: 15px;
  font-weight: bold;
  color: #333;
}

/* Power Button */
.power-circle {
  width: 100px;
  height: 100px;
  border-radius: 50%;
  background: #f2f3f5;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  box-shadow: 0 4px 10px rgba(0,0,0,0.1);
  transition: all 0.3s;
  border: 4px solid white;
}

.power-circle.is-on {
  background: #e8f4ff;
  border-color: #1989fa;
  box-shadow: 0 0 15px rgba(25, 137, 250, 0.3);
}

.power-inner {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.power-inner .icon {
  font-size: 32px;
  margin-bottom: 4px;
  color: #999;
}

.power-circle.is-on .icon {
  color: #1989fa;
}

.power-inner .text {
  font-size: 12px;
  font-weight: bold;
  color: #666;
}

/* --- Mode Bar --- */
.mode-bar {
  display: flex;
  background: white;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 2px 8px rgba(0,0,0,0.05);
  margin-bottom: 10px;
}

.mode-item {
  flex: 1;
  padding: 16px 0;
  text-align: center;
  cursor: pointer;
  transition: all 0.2s;
  border-left: 1px solid #f5f5f5;
}

.mode-item:first-child {
  border-left: none;
}

.mode-item.active {
  background: #1989fa;
  color: white;
}

.mode-name {
  font-size: 16px;
  font-weight: 600;
}

/* --- Footer --- */
.model-tip {
  text-align: center;
  font-size: 12px;
  color: #999;
  padding: 10px 0;
  cursor: pointer;
  text-decoration: underline;
  user-select: none;
}

/* Sheet Styles */
.model-sheet-content {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}
.current-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 16px;
  font-weight: bold;
}
.model-id { color: #1989fa; font-size: 20px; }
.model-actions { display: flex; gap: 10px; }
.model-actions .van-button--primary { flex: 1; }
.model-input-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  border-top: 1px solid #eee;
  padding-top: 10px;
}
.sheet-tip { font-size: 12px; color: #999; line-height: 1.5; }
</style>
