<template>
  <div class="settings-page">
    <!-- 用户信息 -->
    <van-cell-group inset title="用户信息">
      <van-cell title="用户名" :value="authStore.user?.username || '未登录'" />
      <van-cell title="角色" :value="authStore.user?.role || '-'" />
    </van-cell-group>

    <!-- 系统设置 -->
    <van-cell-group inset title="系统设置" class="mt-12">
      <van-cell title="管理员后台" is-link to="/admin">
        <template #right-icon>
          <van-icon name="arrow" />
        </template>
      </van-cell>
      <van-cell title="关于" value="v2.0.0" />
    </van-cell-group>

    <!-- 退出登录 -->
    <van-cell-group inset class="mt-12">
      <van-cell title="退出登录" is-link @click="handleLogout">
        <template #right-icon>
          <van-icon name="arrow" />
        </template>
      </van-cell>
    </van-cell-group>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { showConfirmDialog } from 'vant'

const router = useRouter()
const authStore = useAuthStore()

const handleLogout = async () => {
  try {
    await showConfirmDialog({ message: '确认退出登录？' })
    authStore.logout()
    router.push('/login')
  } catch {
    // 用户取消
  }
}
</script>

<style scoped>
.settings-page {
  padding-bottom: 20px;
}
</style>