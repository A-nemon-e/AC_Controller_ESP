<template>
  <div class="user-management-page">
    <!-- 搜索和操作栏 -->
    <div class="toolbar">
      <van-search
        v-model="searchQuery"
        placeholder="搜索用户名"
        shape="round"
        style="flex: 1"
      />
      <van-button type="primary" icon="plus" @click="showAddUser = true">新增</van-button>
    </div>

    <!-- 用户列表 -->
    <van-pull-refresh v-model="refreshing" @refresh="onRefresh">
      <van-list
        v-model:loading="loading"
        :finished="finished"
        finished-text="没有更多了"
        @load="onLoad"
      >
        <van-cell-group inset>
          <van-swipe-cell v-for="user in filteredUsers" :key="user.id">
            <van-cell
              :title="user.username"
              :label="`注册时间: ${formatDate(user.createdAt)} | 设备: ${user.deviceCount}`"
            >
              <template #right-icon>
                <van-tag :type="user.isActive ? 'success' : 'danger'">
                  {{ user.isActive ? '正常' : '禁用' }}
                </van-tag>
              </template>
            </van-cell>
            
            <template #right>
              <van-button
                square
                type="primary"
                text="编辑"
                @click="editUser(user)"
              />
              <van-button
                square
                type="danger"
                text="删除"
                @click="deleteUser(user)"
              />
            </template>
          </van-swipe-cell>
        </van-cell-group>
      </van-list>
    </van-pull-refresh>

    <!-- 新增/编辑用户弹窗 -->
    <van-dialog
      v-model:show="showAddUser"
      title="新增用户"
      show-cancel-button
      @confirm="saveUser"
    >
      <van-form ref="formRef">
        <van-cell-group inset class="mt-12">
          <van-field
            v-model="userForm.username"
            label="用户名"
            placeholder="请输入用户名"
            :rules="[{ required: true, message: '请填写用户名' }]"
          />
          
          <van-field
            v-model="userForm.password"
            type="password"
            label="密码"
            placeholder="请输入密码"
            :rules="[{ required: true, message: '请填写密码' }]"
          />
          
          <van-field name="role" label="角色">
            <template #input>
              <van-radio-group v-model="userForm.role" direction="horizontal">
                <van-radio name="user">普通用户</van-radio>
                <van-radio name="admin">管理员</van-radio>
              </van-radio-group>
            </template>
          </van-field>
        </van-cell-group>
      </van-form>
    </van-dialog>

    <!-- 编辑用户弹窗 -->
    <van-popup v-model:show="showEditUser" position="bottom" round :style="{ height: '60%' }">
      <van-nav-bar title="编辑用户" left-text="取消" right-text="保存" @click-left="showEditUser = false" @click-right="updateUser" />
      
      <van-cell-group inset class="mt-12">
        <van-cell title="用户名" :value="editingUser?.username" />
        
        <van-cell title="账号状态" center>
          <template #right-icon>
            <van-switch v-model="editForm.isActive" />
          </template>
        </van-cell>
        
        <van-field name="role" label="角色">
          <template #input>
            <van-radio-group v-model="editForm.role" direction="horizontal">
              <van-radio name="user">普通用户</van-radio>
              <van-radio name="admin">管理员</van-radio>
            </van-radio-group>
          </template>
        </van-field>
      </van-cell-group>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useAdminStore } from '@/stores/admin'
import { showToast, showConfirmDialog } from 'vant'
import type { UserWithDevices } from '@/types/admin'

const adminStore = useAdminStore()

// State
const searchQuery = ref('')
const loading = ref(false)
const refreshing = ref(false)
const finished = ref(false)
const showAddUser = ref(false)
const showEditUser = ref(false)
const editingUser = ref<UserWithDevices | null>(null)

const userForm = ref({
  username: '',
  password: '',
  role: 'user' as 'user' | 'admin' | 'superadmin',
})

const editForm = ref<{
  isActive: boolean
  role: string
}>({
  isActive: true,
  role: 'user',
})

// Computed
const filteredUsers = computed(() => {
  if (!searchQuery.value) return adminStore.users
  return adminStore.users.filter(user =>
    user.username.toLowerCase().includes(searchQuery.value.toLowerCase())
  )
})

// Methods
const formatDate = (dateStr: string) => {
  return new Date(dateStr).toLocaleDateString('zh-CN')
}

const onRefresh = async () => {
  await adminStore.fetchUsers()
  refreshing.value = false
  showToast('刷新成功')
}

const onLoad = () => {
  loading.value = false
  finished.value = true
}

const saveUser = async () => {
  if (!userForm.value.username || !userForm.value.password) {
    showToast('请填写完整信息')
    return
  }
  
  await adminStore.createUser(userForm.value)
  showToast('添加成功')
  userForm.value = { username: '', password: '', role: 'user' }
}

const editUser = (user: UserWithDevices) => {
  editingUser.value = user
  editForm.value = {
    isActive: user.isActive,
    role: user.role,
  }
  showEditUser.value = true
}

const updateUser = async () => {
  if (!editingUser.value) return
  
  await adminStore.updateUser(editingUser.value.id, editForm.value)
  showEditUser.value = false
  showToast('更新成功')
}

const deleteUser = async (user: UserWithDevices) => {
  try {
    await showConfirmDialog({
      title: '确认删除',
      message: `确定要删除用户 "${user.username}" 吗？`,
    })
    await adminStore.deleteUser(user.id)
    showToast('删除成功')
  } catch {
    // 取消
  }
}

onMounted(() => {
  adminStore.fetchUsers()
})
</script>

<style scoped>
.user-management-page {
  padding: 12px;
}

.toolbar {
  display: flex;
  gap: 12px;
  margin-bottom: 12px;
  align-items: center;
}

.mt-12 {
  margin-top: 12px;
}
</style>