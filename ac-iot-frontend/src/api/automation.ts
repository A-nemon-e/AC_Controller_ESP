import apiClient from './client'
import type { Schedule, Routine } from '@/types/automation'

export const schedulesApi = {
    getAll: () => apiClient.get<Schedule[]>('/schedules'),

    create: (data: Omit<Schedule, 'id' | 'userId' | 'lastRun'>) =>
        apiClient.post<Schedule>('/schedules', data),

    update: (id: number, data: Partial<Schedule>) =>
        apiClient.put<Schedule>(`/schedules/${id}`, data),

    delete: (id: number) => apiClient.delete(`/schedules/${id}`),
}

export const routinesApi = {
    getAll: () => apiClient.get<Routine[]>('/routines'),

    create: (data: Omit<Routine, 'id' | 'userId' | 'lastTriggered'>) =>
        apiClient.post<Routine>('/routines', data),

    update: (id: number, data: Partial<Routine>) =>
        apiClient.put<Routine>(`/routines/${id}`, data),

    delete: (id: number) => apiClient.delete(`/routines/${id}`),
}
