import apiClient from './client'
import type { LoginRequest, LoginResponse, User } from '@/types/user'

export const authApi = {
    login: (data: LoginRequest) =>
        apiClient.post<LoginResponse>('/auth/login', data),

    register: (username: string, password: string, email?: string) =>
        apiClient.post<User>('/auth/register', { username, password, email }),

    logout: () => apiClient.post('/auth/logout'),
}
