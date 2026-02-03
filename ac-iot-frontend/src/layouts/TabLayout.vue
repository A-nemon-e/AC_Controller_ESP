<template>
  <div class="tab-layout">
    <!-- Header -->
    <div class="header">
      <h2>{{ currentDevice?.name || 'AC IoT Controller' }}</h2>
    </div>

    <!-- 主内容区 -->
    <div class="content">
      <router-view />
    </div>

    <!-- 底部Tab导航 -->
    <van-tabbar v-model="activeTab" @change="onTabChange" fixed>
      <van-tabbar-item name="control" icon="fire">控制</van-tabbar-item>
      <van-tabbar-item name="schedule" icon="clock-o">定时</van-tabbar-item>
      <van-tabbar-item name="routine" icon="notes-o">日程</van-tabbar-item>
      <van-tabbar-item name="settings" icon="setting-o">设置</van-tabbar-item>
    </van-tabbar>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useDevicesStore } from '@/stores/devices'

const router = useRouter()
const route = useRoute()
const devicesStore = useDevicesStore()

const activeTab = ref('control')
const currentDevice = computed(() => devicesStore.currentDevice)

const onTabChange = (name: string | number) => {
  router.push(`/${name}`)
}

// 同步路由和Tab
watch(
  () => route.path,
  (path) => {
    const tab = path.split('/')[1]
    if (tab) activeTab.value = tab
  },
  { immediate: true }
)

// 加载设备列表
onMounted(() => {
  devicesStore.fetchDevices()
})
</script>

<style scoped>
.tab-layout {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: #f7f8fa;
}

.header {
  padding: 16px;
  background: #fff;
  border-bottom: 1px solid #ebedf0;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
}

.header h2 {
  margin: 0;
  font-size: 18px;
  font-weight: 500;
  color: #323233;
}

.content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 50px; /* 为底部Tab留空间 */
}
</style>
