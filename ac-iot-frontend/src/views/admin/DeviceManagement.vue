<template>
  <div class="device-management-page">
    <!-- 搜索和操作栏 -->
    <div class="toolbar">
      <van-search
        v-model="searchQuery"
        placeholder="搜索设备UUID或名称"
        shape="round"
        style="flex: 1"
      />
      <van-button type="primary" icon="scan" @click="scanDevices">扫描</van-button>
    </div>

    <!-- 筛选标签 -->
    <div class="filter-tabs">
      <van-tabs v-model:active="activeTab" swipeable>
        <van-tab title="全部"></van-tab>
        <van-tab title="在线"></van-tab>
        <van-tab title="离线"></van-tab>
      </van-tabs>
    </div>

    <!-- 设备列表 -->
    <van-pull-refresh v-model="refreshing" @refresh="onRefresh">
      <van-list
        v-model:loading="loading"
        :finished="finished"
        finished-text="没有更多了"
        @load="onLoad"
      >
        <van-cell-group inset>
          <van-cell
            v-for="device in filteredDevices"
            :key="device.id"
            :title="device.name"
            :label="`${device.uuid} | 固件: ${device.firmwareVersion || '未知'}`"
            is-link
            @click="viewDevice(device)"
          >
            <template #right-icon>
              <van-tag :type="device.isOnline === true ? 'success' : 'danger'">
                {{ device.isOnline === true ? '在线' : '离线' }}
              </van-tag>
            </template>
          </van-cell>
        </van-cell-group>
        
        <van-empty v-if="filteredDevices.length === 0" description="暂无设备" />
      </van-list>
    </van-pull-refresh>

    <!-- 设备详情弹窗 -->
    <van-popup v-model:show="showDeviceDetail" position="bottom" round :style="{ height: '70%' }">
      <div v-if="selectedDevice" class="device-detail">
        <van-nav-bar title="设备详情" left-text="关闭" @click-left="showDeviceDetail = false" />
        
        <van-cell-group inset class="mt-12">
          <van-cell title="设备名称" :value="selectedDevice.name" />
          <van-cell title="UUID" :value="selectedDevice.uuid" />
          <van-cell title="MAC地址" :value="selectedDevice.mac || '未知'" />
          <van-cell title="IP地址" :value="selectedDevice.ip || '未知'" />
          <van-cell title="固件版本" :value="selectedDevice.firmwareVersion || '未知'" />
          <van-cell title="状态" :value="selectedDevice.isOnline === true ? '在线' : '离线'" />
          <van-cell title="所有者" :value="selectedDevice.ownerName" />
          <van-cell title="创建时间" :value="formatDate(selectedDevice.createdAt)" />
        </van-cell-group>

        <div class="action-buttons">
          <van-button type="primary" block @click="rebootDevice">重启设备</van-button>
          <van-button type="warning" block @click="resetDevice">重置设备</van-button>
          <van-button type="danger" block @click="deleteDevice">删除设备</van-button>
        </div>
      </div>
    </van-popup>

    <!-- 扫描弹窗 -->
    <van-popup v-model:show="showScan" round closeable>
      <div class="scan-popup">
        <h3>扫描可用设备</h3>
        <van-loading v-if="scanning" size="40px">扫描中...</van-loading>
        <van-empty v-else-if="scannedDevices.length === 0" description="未发现设备" />
        <van-cell-group v-else inset>
          <van-cell
            v-for="device in scannedDevices"
            :key="device.uuid"
            :title="device.uuid"
            :label="`MAC: ${device.mac} | RSSI: ${device.rssi}`"
          >
            <template #right-icon>
              <van-button size="small" type="primary" @click="addDevice(device)">添加</van-button>
            </template>
          </van-cell>
        </van-cell-group>
      </div>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useAdminStore } from '@/stores/admin'
import { showToast, showConfirmDialog } from 'vant'
import type { DeviceWithUser } from '@/types/admin'

const adminStore = useAdminStore()

// State
const searchQuery = ref('')
const activeTab = ref(0)
const loading = ref(false)
const refreshing = ref(false)
const finished = ref(false)
const showDeviceDetail = ref(false)
const selectedDevice = ref<DeviceWithUser | null>(null)
const showScan = ref(false)
const scanning = ref(false)
const scannedDevices = ref<any[]>([])

// Computed
const filteredDevices = computed(() => {
  let devices = adminStore.devices
  
  // 搜索过滤
  if (searchQuery.value) {
    devices = devices.filter(d =>
      d.name.toLowerCase().includes(searchQuery.value.toLowerCase()) ||
      d.uuid.toLowerCase().includes(searchQuery.value.toLowerCase())
    )
  }
  
  // 状态过滤
  if (activeTab.value === 1) {
    devices = devices.filter(d => d.isOnline === true)
  } else if (activeTab.value === 2) {
    devices = devices.filter(d => d.isOnline !== true)
  }
  
  return devices
})

// Methods
const formatDate = (dateStr: string | undefined) => {
  if (!dateStr) return '未知'
  return new Date(dateStr).toLocaleDateString('zh-CN')
}

const onRefresh = async () => {
  await adminStore.fetchDevices()
  refreshing.value = false
  showToast('刷新成功')
}

const onLoad = () => {
  loading.value = false
  finished.value = true
}

const viewDevice = (device: DeviceWithUser) => {
  selectedDevice.value = device
  showDeviceDetail.value = true
}

const scanDevices = () => {
  showScan.value = true
  scanning.value = true
  scannedDevices.value = []
  
  // 模拟扫描
  setTimeout(() => {
    scanning.value = false
    scannedDevices.value = [
      { uuid: 'ESP_003', mac: 'AA:BB:CC:DD:EE:03', rssi: -45 },
      { uuid: 'ESP_004', mac: 'AA:BB:CC:DD:EE:04', rssi: -62 },
    ]
  }, 2000)
}

const addDevice = (device: any) => {
  showToast(`添加设备 ${device.uuid}`)
  showScan.value = false
}

const rebootDevice = async () => {
  try {
    await showConfirmDialog({ message: '确定要重启设备吗？' })
    showToast('重启指令已发送')
  } catch {
    // 取消
  }
}

const resetDevice = async () => {
  try {
    await showConfirmDialog({ message: '确定要重置设备吗？这将清除所有配置！' })
    showToast('重置指令已发送')
  } catch {
    // 取消
  }
}

const deleteDevice = async () => {
  if (!selectedDevice.value) return
  
  try {
    await showConfirmDialog({
      title: '确认删除',
      message: `确定要删除设备 "${selectedDevice.value.name}" 吗？`,
    })
    showToast('删除成功')
    showDeviceDetail.value = false
  } catch {
    // 取消
  }
}

onMounted(() => {
  adminStore.fetchDevices()
})
</script>

<style scoped>
.device-management-page {
  padding: 12px;
}

.toolbar {
  display: flex;
  gap: 12px;
  margin-bottom: 12px;
  align-items: center;
}

.filter-tabs {
  margin-bottom: 12px;
}

.mt-12 {
  margin-top: 12px;
}

.action-buttons {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.scan-popup {
  padding: 20px;
  min-height: 300px;
}

.scan-popup h3 {
  text-align: center;
  margin-bottom: 20px;
}
</style>