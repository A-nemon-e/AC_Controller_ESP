import axios from 'axios'
import type { LoginRequest, LoginResponse } from '@/types/user'

const apiClient = axios.create({
    baseURL: '/api',
    timeout: 10000,
    headers: {
        'Content-Type': 'application/json',
    },
})

// 请求拦截器 - 添加token
apiClient.interceptors.request.use((config) => {
    // 从sessionStorage获取token（安全存储方案）
    try {
        const token = sessionStorage.getItem('access_token')
        if (token) {
            config.headers.Authorization = `Bearer ${token}`
        }
    } catch (e) {
        console.warn('sessionStorage not available')
    }
    return config
})

export default apiClient

export const authApi = {
    login: async (credentials: LoginRequest): Promise<LoginResponse> => {
        const response = await apiClient.post('/auth/login', credentials)
        return response.data
    },
    register: async (username: string, password: string) => {
        const response = await apiClient.post('/auth/register', { username, password })
        return response.data
    },
}