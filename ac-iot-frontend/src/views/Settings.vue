<template>
  <div class="settings-page">
    <!-- 设备管理 -->
    <van-cell-group inset title="设备管理">
      <van-cell
        v-for="device in devices"
        :key="device.id"
        :title="device.name"
        :label="`UUID: ${device.uuid}`"
        is-link
        @click="viewDevice(device.id)"
      />
      <van-cell title="添加新设备" is-link icon="plus" @click="showAddDevice = true" />
    </van-cell-group>

    <!-- 用户设置 -->
    <van-cell-group inset title="用户设置">
      <van-cell title="退出登录" is-link @click="handleLogout" />
    </van-cell-group>

    <!-- 关于 -->
    <van-cell-group inset title="关于">
      <van-cell title="版本" value="1.0.0" />
      <van-cell title="作者" value="AC IoT Team" />
    </van-cell-group>

    <!-- 添加设备弹出层 -->
    <van-popup v-model:show="showAddDevice" position="bottom" :style="{ height: '50%' }">
      <div class="add-device-form">
        <h3>添加设备</h3>
        <van-form @submit="onAddDevice">
          <van-cell-group inset>
            <van-field
              v-model="newDevice.uuid"
              label="设备UUID"
              placeholder="ESP_XXXXXXXXXXXX"
              :rules="[{ required: true, message: '请输入设备UUID' }]"
            />
            <van-field
              v-model="newDevice.name"
              label="设备名称"
              placeholder="例如：客厅空调"
              :rules="[{ required: true, message: '请输入设备名称' }]"
            />
          </van-cell-group>

          <div class="form-buttons">
            <van-button block @click="showAddDevice = false">取消</van-button>
            <van-button block type="primary" native-type="submit" :loading="adding">
              添加
            </van-button>
          </div>
        </van-form>

        <van-divider>或者</van-divider>

        <van-button block type="success" @click="showDiscovery = true">
          🔍 扫描可用设备
        </van-button>
      </div>
    </van-popup>

    <!-- 设备发现弹出层 -->
    <van-popup v-model:show="showDiscovery" position="bottom" :style="{ height: '60%' }">
      <div class="discovery-panel">
        <h3>发现设备</h3>
        
        <van-button type="primary" size="small" @click="refreshDiscovery" :loading="discovering">
          🔄 刷新
        </van-button>

        <van-empty v-if="discoveredDevices.length === 0" description="未发现可用设备">
          <van-button type="primary" @click="refreshDiscovery">刷新列表</van-button>
        </van-empty>

        <van-cell-group v-else inset>
          <van-cell
            v-for="device in discoveredDevices"
            :key="device.uuid"
            :title="device.uuid"
            :label="`IP: ${device.ip} | MAC: ${device.mac}`"
          >
            <template #right-icon>
              <van-button size="small" type="primary" @click="addDiscoveredDevice(device)">
                添加
              </van-button>
            </template>
          </van-cell>
        </van-cell-group>
      </div>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { showToast, showConfirmDialog } from 'vant'
import { useAuthStore } from '@/stores/auth'
import { useDevicesStore } from '@/stores/devices'
import { devicesApi } from '@/api/devices'
import type { DiscoveredDevice } from '@/types/device'

const router = useRouter()
const authStore = useAuthStore()
const devicesStore = useDevicesStore()

const devices = computed(() => devicesStore.devices)
const showAddDevice = ref(false)
const showDiscovery = ref(false)
const adding = ref(false)
const discovering = ref(false)
const discoveredDevices = ref<DiscoveredDevice[]>([])

const newDevice = ref({
  uuid: '',
  name: '',
})

const viewDevice = (id: number) => {
  // TODO: 跳转到设备详情页
  showToast('设备详情功能开发中')
}

const handleLogout = async () => {
  try {
    await showConfirmDialog({ message: '确认退出登录？' })
    authStore.logout()
    router.push('/login')
  } catch  {
    // 用户取消
  }
}

const onAddDevice = async () => {
  adding.value = true
  try {
    const device = await devicesApi.create(newDevice.value)
    devicesStore.addDevice(device)
    showToast('添加成功')
    showAddDevice.value = false
    newDevice.value = { uuid: '', name: '' }
  } catch (error) {
    showToast('添加失败')
  } finally {
    adding.value = false
  }
}

const refreshDiscovery = async () => {
  discovering.value = true
  try {
    const result = await devicesApi.getDiscoveredDevices()
    discoveredDevices.value = result.devices
    if (result.count === 0) {
      showToast('未发现可用设备')
    } else {
      showToast(`发现 ${result.count} 个设备`)
    }
  } catch (error) {
    showToast('扫描失败')
  } finally {
    discovering.value = false
  }
}

const addDiscoveredDevice = async (device: DiscoveredDevice) => {
  const name = prompt('请输入设备名称', '客厅空调')
  if (!name) return

  adding.value = true
  try {
    const newDev = await devicesApi.create({ uuid: device.uuid, name })
    devicesStore.addDevice(newDev)
    showToast('添加成功')
    showDiscovery.value = false
    await devicesStore.fetchDevices()
  } catch (error) {
    showToast('添加失败')
  } finally {
    adding.value = false
  }
}

onMounted(() => {
  devicesStore.fetchDevices()
})
</script>

<style scoped>
.settings-page {
  padding-bottom: 20px;
}

.add-device-form,
.discovery-panel {
  padding: 16px;
}

.add-device-form h3,
.discovery-panel h3 {
  margin-bottom: 16px;
  text-align: center;
}

.form-buttons {
  display: flex;
  gap: 12px;
  margin-top: 24px;
}

.discovery-panel .van-button {
  margin-bottom: 16px;
}
</style>
