<template>
  <div class="admin-layout">
    <!-- 移动端顶部导航 -->
    <van-nav-bar
      title="管理后台"
      left-arrow
      @click-left="goBack"
      fixed
    >
      <template #right>
        <van-icon name="wap-nav" size="20" @click="showMenu = true" />
      </template>
    </van-nav-bar>

    <!-- 主内容区 -->
    <div class="admin-content">
      <router-view />
    </div>

    <!-- 侧边菜单弹窗 -->
    <van-popup
      v-model:show="showMenu"
      position="right"
      :style="{ width: '70%', height: '100%' }"
    >
      <div class="admin-menu">
        <div class="menu-header">
          <div class="admin-avatar">
            <van-icon name="manager-o" size="40" />
          </div>
          <div class="admin-info">
            <div class="admin-name">{{ authStore.user?.username || '管理员' }}</div>
            <van-tag type="primary">{{ authStore.user?.role || 'admin' }}</van-tag>
          </div>
        </div>

        <van-cell-group inset class="menu-list">
          <van-cell
            v-for="item in menuItems"
            :key="item.path"
            :title="item.title"
            :icon="item.icon"
            is-link
            :to="item.path"
            @click="showMenu = false"
          />
        </van-cell-group>

        <div class="menu-footer">
          <van-button block @click="goBack">
            <van-icon name="revoke" /> 返回前台
          </van-button>
        </div>
      </div>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()

const showMenu = ref(false)

const menuItems = [
  { title: '仪表盘', icon: 'chart-trending-o', path: '/admin/dashboard' },
  { title: '用户管理', icon: 'user-o', path: '/admin/users' },
  { title: '设备管理', icon: 'tv-o', path: '/admin/devices' },
  { title: 'OTA管理', icon: 'upgrade', path: '/admin/ota' },
]

const goBack = () => {
  router.push('/')
}
</script>

<style scoped>
.admin-layout {
  min-height: 100vh;
  background: #f7f8fa;
}

.admin-content {
  padding-top: 46px;
  padding-bottom: 20px;
}

.admin-menu {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f7f8fa;
}

.menu-header {
  padding: 40px 20px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  display: flex;
  align-items: center;
  gap: 16px;
}

.admin-avatar {
  width: 60px;
  height: 60px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.admin-info {
  flex: 1;
}

.admin-name {
  font-size: 18px;
  font-weight: 500;
  margin-bottom: 4px;
}

.menu-list {
  flex: 1;
  margin-top: 20px;
}

.menu-footer {
  padding: 20px;
}
</style>