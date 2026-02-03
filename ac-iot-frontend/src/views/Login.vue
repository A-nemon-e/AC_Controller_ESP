<template>
  <div class="login-page">
    <div class="login-container">
      <div class="logo">
        <h1>🌡️ AC IoT</h1>
        <p>空调智能控制系统</p>
      </div>

      <van-form @submit="onSubmit">
        <van-cell-group inset>
          <van-field
            v-model="formData.username"
            name="username"
            label="用户名"
            placeholder="请输入用户名"
            :rules="[{ required: true, message: '请填写用户名' }]"
          />
          <van-field
            v-model="formData.password"
            type="password"
            name="password"
            label="密码"
            placeholder="请输入密码"
            :rules="[{ required: true, message: '请填写密码' }]"
          />
        </van-cell-group>

        <div class="form-footer">
          <van-checkbox v-model="remember">记住我</van-checkbox>
        </div>

        <div class="submit-btn">
          <van-button
            round
            block
            type="primary"
            native-type="submit"
            :loading="authStore.loading"
            size="large"
          >
            登录
          </van-button>
        </div>
      </van-form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { showToast } from 'vant'

const router = useRouter()
const authStore = useAuthStore()

const formData = reactive({
  username: '',
  password: '',
})
const remember = ref(false)

const onSubmit = async () => {
  const success = await authStore.login(formData)

  if (success) {
    showToast('登录成功')
    router.push('/')
  } else {
    showToast('登录失败，请检查用户名和密码')
  }
}
</script>

<style scoped>
.login-page {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.login-container {
  width: 100%;
  max-width: 400px;
}

.logo {
  text-align: center;
  margin-bottom: 48px;
  color: white;
}

.logo h1 {
  font-size: 48px;
  margin: 0 0 8px 0;
}

.logo p {
  font-size: 16px;
  opacity: 0.9;
  margin: 0;
}

.form-footer {
  padding: 16px 16px 0 16px;
}

.submit-btn {
  margin: 24px 16px 16px 16px;
}
</style>
