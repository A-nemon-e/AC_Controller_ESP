import { defineStore } from 'pinia'
import { ref } from 'vue'
import { authApi } from '@/api/auth'
import type { User, LoginRequest } from '@/types/user'

/**
 * JWT安全存储方案
 * 使用内存存储 + sessionStorage回退
 * 避免localStorage的XSS风险
 */
export const useAuthStore = defineStore('auth', () => {
    const user = ref<User | null>(null)
    // 使用内存存储Token，避免XSS攻击
    let memoryToken: string | null = null
    
    // 初始化时尝试从sessionStorage恢复（页面刷新时）
    const initToken = () => {
        try {
            memoryToken = sessionStorage.getItem('access_token')
        } catch (e) {
            console.warn('sessionStorage not available')
        }
    }
    initToken()
    
    const token = ref<string | null>(memoryToken)
    const loading = ref(false)

    const login = async (credentials: LoginRequest) => {
        loading.value = true
        try {
            const response = await authApi.login(credentials)
            token.value = response.access_token
            memoryToken = response.access_token
            // 使用sessionStorage替代localStorage
            try {
                sessionStorage.setItem('access_token', response.access_token)
            } catch (e) {
                console.warn('sessionStorage not available')
            }
            return true
        } catch (error) {
            console.error('Login failed:', error)
            return false
        } finally {
            loading.value = false
        }
    }

    const register = async (credentials: LoginRequest) => {
        loading.value = true
        try {
            await authApi.register(credentials.username, credentials.password)
            return await login(credentials)
        } catch (error) {
            console.error('Register failed:', error)
            return false
        } finally {
            loading.value = false
        }
    }

    const logout = () => {
        user.value = null
        token.value = null
        memoryToken = null
        try {
            sessionStorage.removeItem('access_token')
        } catch (e) {
            console.warn('sessionStorage not available')
        }
    }

    const isAuthenticated = () => !!token.value

    // 获取Token（用于API调用）
    const getToken = () => memoryToken || token.value

    return {
        user,
        token,
        loading,
        login,
        register,
        logout,
        isAuthenticated,
        getToken,
    }
})