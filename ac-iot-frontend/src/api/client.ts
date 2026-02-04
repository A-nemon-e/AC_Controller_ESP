import axios from 'axios'
import type { AxiosInstance } from 'axios'

const getBaseUrl = () => {
    const url = import.meta.env.VITE_API_URL || ''
    if (url.endsWith('/api')) {
        return url
    }
    // 如果url不为空且不以/结尾，加上/
    const prefix = url && !url.endsWith('/') ? '/' : ''
    return `${url}${prefix}api`
}

const apiClient: AxiosInstance = axios.create({
    baseURL: getBaseUrl(),
    timeout: 10000,
})

// 请求拦截器 - 添加Token
apiClient.interceptors.request.use(
    (config) => {
        const token = localStorage.getItem('access_token')
        if (token) {
            config.headers.Authorization = `Bearer ${token}`
        }
        return config
    },
    (error) => {
        return Promise.reject(error)
    }
)

// 响应拦截器 - 处理401
apiClient.interceptors.response.use(
    (response) => response.data,
    (error) => {
        if (error.response?.status === 401) {
            localStorage.removeItem('access_token')
            window.location.href = '/login'
        }
        return Promise.reject(error)
    }
)

export default apiClient
