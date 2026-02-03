import { defineStore } from 'pinia'
import { ref } from 'vue'
import { authApi } from '@/api/auth'
import type { User, LoginRequest } from '@/types/user'

export const useAuthStore = defineStore('auth', () => {
    const user = ref<User | null>(null)
    const token = ref<string | null>(localStorage.getItem('access_token'))
    const loading = ref(false)

    const login = async (credentials: LoginRequest) => {
        loading.value = true
        try {
            const response = await authApi.login(credentials)
            token.value = response.access_token
            localStorage.setItem('access_token', response.access_token)
            return true
        } catch (error) {
            console.error('Login failed:', error)
            return false
        } finally {
            loading.value = false
        }
    }

    const logout = () => {
        user.value = null
        token.value = null
        localStorage.removeItem('access_token')
    }

    const isAuthenticated = () => !!token.value

    return {
        user,
        token,
        loading,
        login,
        logout,
        isAuthenticated,
    }
})
