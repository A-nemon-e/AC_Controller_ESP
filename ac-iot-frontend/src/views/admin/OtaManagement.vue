<template>
  <div class="ota-management-page">
    <!-- OTA统计 -->
    <div class="stats-grid">
      <div class="stat-card">
        <div class="stat-value">{{ otaPackages.length }}</div>
        <div class="stat-label">固件版本</div>
      </div>
      
      <div class="stat-card">
        <div class="stat-value">{{ pendingUpdates.length }}</div>
        <div class="stat-label">待更新</div>
      </div>
      
      <div class="stat-card">
        <div class="stat-value">{{ completedUpdates.length }}</div>
        <div class="stat-label">已更新</div>
      </div>
      
      <div class="stat-card">
        <div class="stat-value">{{ failedUpdates.length }}</div>
        <div class="stat-label">失败</div>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div class="action-bar">
      <van-button type="primary" icon="plus" block @click="showAddPackage = true">发布新版本</van-button>
    </div>

    <!-- 版本列表 -->
    <van-cell-group inset title="固件版本" class="mt-12">
      <van-collapse v-model="activeCollapse">
        <van-collapse-item
          v-for="pkg in otaPackages"
          :key="pkg.id"
          :title="`v${pkg.version}`"
          :value="pkg.status"
        >
          <div class="package-detail">
            <div class="detail-row">
              <span class="label">版本号:</span>
              <span>{{ pkg.version }}</span>
            </div>
            
            <div class="detail-row">
              <span class="label">文件大小:</span>
              <span>{{ formatFileSize(pkg.fileSize) }}</span>
            </div>
            
            <div class="detail-row">
              <span class="label">发布时间:</span>
              <span>{{ formatDate(pkg.releaseDate) }}</span>
            </div>
            
            <div class="detail-row">
              <span class="label">目标设备:</span>
              <span>{{ pkg.targetDevices.length }} 个型号</span>
            </div>
            
            <div class="detail-row">
              <span class="label">强制更新:</span>
              <van-switch v-model="pkg.isMandatory" size="20" disabled />
            </div>

            <div class="changelog">
              <div class="label">更新日志:</div>
              <pre>{{ pkg.changelog }}</pre>
            </div>

            <div class="package-actions">
              <van-button size="small" type="primary" @click="viewProgress(pkg)">查看进度</van-button>
              <van-button size="small" type="danger" @click="archivePackage(pkg)">归档</van-button>
            </div>
          </div>
        </van-collapse-item>
      </van-collapse>

      <van-empty v-if="otaPackages.length === 0" description="暂无固件版本" />
    </van-cell-group>

    <!-- 添加版本弹窗 -->
    <van-dialog
      v-model:show="showAddPackage"
      title="发布新版本"
      show-cancel-button
      @confirm="savePackage"
    >
      <van-cell-group inset class="mt-12">
        <van-field
          v-model="packageForm.version"
          label="版本号"
          placeholder="例如: 1.2.0"
        />

        <van-field
          v-model="packageForm.changelog"
          label="更新日志"
          type="textarea"
          rows="3"
          placeholder="输入更新内容"
        />

        <van-cell title="强制更新" center>
          <template #right-icon>
            <van-switch v-model="packageForm.isMandatory" />
          </template>
        </van-cell>

        <van-cell title="灰度发布" center>
          <template #right-icon>
            <van-switch v-model="packageForm.useRollout" />
          </template>
        </van-cell>

        <van-cell
          v-if="packageForm.useRollout"
          title="灰度比例"
          :value="`${packageForm.rolloutPercentage}%`"
        >
          <template #right-icon>
            <van-slider v-model="packageForm.rolloutPercentage" :min="1" :max="100" style="width: 120px" />
          </template>
        </van-cell>

        <van-uploader
          v-model="packageForm.fileList"
          :max-count="1"
          accept=".bin"
          :after-read="afterRead"
        >
          <van-button icon="plus" type="primary">上传固件</van-button>
        </van-uploader>
      </van-cell-group>
    </van-dialog>

    <!-- 更新进度弹窗 -->
    <van-popup v-model:show="showProgress" round position="bottom" :style="{ height: '70%' }">
      <div class="progress-popup">
        <van-nav-bar title="更新进度" left-text="关闭" @click-left="showProgress = false" />
        
        <van-cell-group inset class="mt-12">
          <van-cell
            v-for="update in updateProgress"
            :key="update.id"
            :title="update.deviceUuid"
            :label="updateStatusText(update.status)"
          >
            <template #right-icon>
              <van-circle
                v-if="update.status === 'downloading' || update.status === 'installing'"
                v-model:current-rate="update.progress"
                :rate="update.progress"
                size="40px"
                :stroke-width="60"
                :text="`${update.progress}%`"
              />
              <van-tag v-else :type="getUpdateStatusType(update.status)">
                {{ updateStatusText(update.status) }}
              </van-tag>
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
import { showToast } from 'vant'
import type { OtaPackage, OtaUpdate } from '@/types/admin'

const adminStore = useAdminStore()

// State
const activeCollapse = ref<string[]>([])
const showAddPackage = ref(false)
const showProgress = ref(false)

const packageForm = ref({
  version: '',
  changelog: '',
  isMandatory: false,
  useRollout: false,
  rolloutPercentage: 10,
  fileList: [],
})

const otaPackages = ref<OtaPackage[]>([
  {
    id: '1',
    version: '1.2.0',
    firmwareUrl: 'https://example.com/firmware/v1.2.0.bin',
    changelog: '- 优化显示性能\n- 修复内存泄漏问题\n- 新增天气动画效果',
    fileSize: 1024 * 1024,
    targetDevices: ['ESP32'],
    releaseDate: '2024-03-10',
    isMandatory: false,
    rolloutPercentage: 100,
    status: 'published',
  },
  {
    id: '2',
    version: '1.1.0',
    firmwareUrl: 'https://example.com/firmware/v1.1.0.bin',
    changelog: '- 新增卡片编辑功能\n- 优化网络连接',
    fileSize: 980 * 1024,
    targetDevices: ['ESP32'],
    releaseDate: '2024-02-20',
    isMandatory: true,
    rolloutPercentage: 100,
    status: 'published',
  },
])

const updateProgress = ref([
  { id: '1', packageId: '1', deviceId: 1, deviceUuid: 'ESP_001', status: 'completed' as OtaUpdate['status'], progress: 100 },
  { id: '2', packageId: '1', deviceId: 2, deviceUuid: 'ESP_002', status: 'downloading' as OtaUpdate['status'], progress: 45 },
  { id: '3', packageId: '1', deviceId: 3, deviceUuid: 'ESP_003', status: 'pending' as OtaUpdate['status'], progress: 0 },
  { id: '4', packageId: '1', deviceId: 4, deviceUuid: 'ESP_004', status: 'installing' as OtaUpdate['status'], progress: 75 },
  { id: '5', packageId: '1', deviceId: 5, deviceUuid: 'ESP_005', status: 'failed' as OtaUpdate['status'], progress: 0 },
])

// Computed
const pendingUpdates = computed(() => 
  updateProgress.value.filter(u => ['pending', 'downloading', 'installing'].includes(u.status))
)

const completedUpdates = computed(() => 
  updateProgress.value.filter(u => u.status === 'completed')
)

const failedUpdates = computed(() => 
  updateProgress.value.filter(u => u.status === 'failed')
)

// Methods
const formatFileSize = (bytes: number) => {
  if (bytes < 1024) return bytes + ' B'
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB'
  return (bytes / (1024 * 1024)).toFixed(2) + ' MB'
}

const formatDate = (dateStr: string) => {
  return new Date(dateStr).toLocaleDateString('zh-CN')
}

const updateStatusText = (status: string) => {
  const texts: Record<string, string> = {
    'pending': '等待中',
    'downloading': '下载中',
    'installing': '安装中',
    'completed': '已完成',
    'failed': '失败',
    'rolled_back': '已回滚',
  }
  return texts[status] || status
}

type TagType = 'primary' | 'success' | 'warning' | 'danger' | 'default'

const getUpdateStatusType = (status: string): TagType => {
  const types: Record<string, TagType> = {
    'pending': 'primary',
    'downloading': 'warning',
    'installing': 'warning',
    'completed': 'success',
    'failed': 'danger',
    'rolled_back': 'default',
  }
  return types[status] || 'default'
}

const afterRead = (file: any) => {
  showToast(`已选择文件: ${file.file.name}`)
}

const savePackage = () => {
  if (!packageForm.value.version) {
    showToast('请输入版本号')
    return
  }
  
  const newPackage: OtaPackage = {
    id: String(Date.now()),
    version: packageForm.value.version,
    firmwareUrl: '',
    changelog: packageForm.value.changelog,
    fileSize: 1024 * 1024,
    targetDevices: ['ESP32'],
    releaseDate: new Date().toISOString(),
    isMandatory: packageForm.value.isMandatory,
    rolloutPercentage: packageForm.value.useRollout ? packageForm.value.rolloutPercentage : 100,
    status: 'draft',
  }
  
  otaPackages.value.unshift(newPackage)
  showToast('版本发布成功')
  packageForm.value = {
    version: '',
    changelog: '',
    isMandatory: false,
    useRollout: false,
    rolloutPercentage: 10,
    fileList: [],
  }
}

const viewProgress = (_pkg: OtaPackage) => {
  showProgress.value = true
}

const archivePackage = async (pkg: OtaPackage) => {
  const index = otaPackages.value.findIndex(p => p.id === pkg.id)
  if (index !== -1) {
    otaPackages.value[index].status = 'archived'
    showToast('已归档')
  }
}

onMounted(() => {
  adminStore.fetchOtaPackages()
})
</script>

<style scoped>
.ota-management-page {
  padding: 12px;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 8px;
  margin-bottom: 12px;
}

.stat-card {
  background: white;
  border-radius: 8px;
  padding: 12px 8px;
  text-align: center;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
}

.stat-value {
  font-size: 20px;
  font-weight: 600;
  color: #323233;
}

.stat-label {
  font-size: 11px;
  color: #969799;
  margin-top: 2px;
}

.action-bar {
  margin-bottom: 12px;
}

.mt-12 {
  margin-top: 12px;
}

.package-detail {
  padding: 12px;
}

.detail-row {
  display: flex;
  margin-bottom: 8px;
  font-size: 14px;
}

.label {
  color: #969799;
  width: 80px;
  flex-shrink: 0;
}

.changelog {
  margin: 12px 0;
}

.changelog pre {
  background: #f7f8fa;
  padding: 12px;
  border-radius: 4px;
  font-size: 13px;
  white-space: pre-wrap;
  margin-top: 8px;
}

.package-actions {
  display: flex;
  gap: 12px;
  margin-top: 16px;
}

.progress-popup {
  height: 100%;
  display: flex;
  flex-direction: column;
}

:deep(.van-collapse-item__content) {
  padding: 0;
}
</style>