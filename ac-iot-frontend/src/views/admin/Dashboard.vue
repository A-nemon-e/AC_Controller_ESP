<template>
  <div class="dashboard-page">
    <!-- 统计卡片 -->
    <div class="stats-grid">
      <div class="stat-card">
        <div class="stat-icon"><van-icon name="user-o" /></div>
        <div class="stat-content">
          <div class="stat-value">{{ adminStore.stats.totalUsers }}</div>
          <div class="stat-label">总用户数</div>
        </div>
      </div>
      
      <div class="stat-card">
        <div class="stat-icon blue"><van-icon name="tv-o" /></div>
        <div class="stat-content">
          <div class="stat-value">{{ adminStore.stats.totalDevices }}</div>
          <div class="stat-label">设备总数</div>
        </div>
      </div>
      
      <div class="stat-card">
        <div class="stat-icon green"><van-icon name="success" /></div>
        <div class="stat-content">
          <div class="stat-value">{{ adminStore.stats.onlineDevices }}</div>
          <div class="stat-label">在线设备</div>
        </div>
      </div>
      
      <div class="stat-card">
        <div class="stat-icon orange"><van-icon name="fire-o" /></div>
        <div class="stat-content">
          <div class="stat-value">{{ adminStore.stats.activeToday }}</div>
          <div class="stat-label">今日活跃</div>
        </div>
      </div>
    </div>

    <!-- 在线率 -->
    <van-cell-group inset title="设备在线率" class="mt-12">
      <div class="online-rate">
        <van-circle
          v-model:current-rate="onlineRate"
          :rate="onlineRate"
          :speed="100"
          :text="`${onlineRate.toFixed(1)}%`"
          :stroke-width="60"
          size="120px"
          color="#07c160"
        />
        <div class="rate-info">
          <div class="rate-item">
            <span class="dot online"></span>
            <span>在线: {{ adminStore.stats.onlineDevices }}</span>
          </div>
          <div class="rate-item">
            <span class="dot offline"></span>
            <span>离线: {{ adminStore.stats.totalDevices - adminStore.stats.onlineDevices }}</span>
          </div>
        </div>
      </div>
    </van-cell-group>

    <!-- 快捷入口 -->
    <van-cell-group inset title="快捷入口" class="mt-12">
      <van-grid :column-num="4">
        <van-grid-item
          v-for="item in quickActions"
          :key="item.path"
          :icon="item.icon"
          :text="item.title"
          @click="$router.push(item.path)"
        />
      </van-grid>
    </van-cell-group>

    <!-- 最近活动 -->
    <van-cell-group inset title="最近活动" class="mt-12">
      <van-cell
        v-for="(log, index) in recentLogs"
        :key="index"
        :title="log.message"
        :label="log.timestamp"
      >
        <template #right-icon>
          <van-tag :type="log.level === 'error' ? 'danger' : 'primary'">
            {{ log.level }}
          </van-tag>
        </template>
      </van-cell>
      
      <van-empty v-if="recentLogs.length === 0" description="暂无活动记录" />
    </van-cell-group>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { useAdminStore } from '@/stores/admin'

const adminStore = useAdminStore()

const onlineRate = computed(() => {
  if (adminStore.stats.totalDevices === 0) return 0
  return (adminStore.stats.onlineDevices / adminStore.stats.totalDevices) * 100
})

const quickActions = [
  { title: '用户管理', icon: 'user-o', path: '/admin/users' },
  { title: '设备管理', icon: 'tv-o', path: '/admin/devices' },
  { title: 'OTA管理', icon: 'upgrade', path: '/admin/ota' },
  { title: '系统日志', icon: 'records', path: '/admin/logs' },
]

const recentLogs = computed(() => [
  { message: '用户 user1 登录成功', timestamp: '2024-03-13 10:30:00', level: 'info' },
  { message: '设备 ESP_001 上线', timestamp: '2024-03-13 10:25:00', level: 'info' },
  { message: '系统备份完成', timestamp: '2024-03-13 03:00:00', level: 'info' },
])

onMounted(() => {
  adminStore.fetchStats()
})
</script>

<style scoped>
.dashboard-page {
  padding: 12px;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
  margin-bottom: 12px;
}

.stat-card {
  background: white;
  border-radius: 8px;
  padding: 16px;
  display: flex;
  align-items: center;
  gap: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
}

.stat-icon {
  width: 48px;
  height: 48px;
  background: #e3f2fd;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #1989fa;
  font-size: 24px;
}

.stat-icon.blue {
  background: #e8f5e9;
  color: #07c160;
}

.stat-icon.green {
  background: #f3e5f5;
  color: #9c27b0;
}

.stat-icon.orange {
  background: #fff3e0;
  color: #ff9800;
}

.stat-content {
  flex: 1;
}

.stat-value {
  font-size: 24px;
  font-weight: 600;
  color: #323233;
  line-height: 1.2;
}

.stat-label {
  font-size: 12px;
  color: #969799;
  margin-top: 4px;
}

.online-rate {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 32px;
  padding: 24px;
}

.rate-info {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.rate-item {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 14px;
  color: #666;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.dot.online {
  background: #07c160;
}

.dot.offline {
  background: #969799;
}

.mt-12 {
  margin-top: 12px;
}
</style>