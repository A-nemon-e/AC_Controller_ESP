import apiClient from './client'
import type { OtaPackage } from '@/types/admin'

export const adminApi = {
  getStats: () =>
    apiClient.get('/admin/stats/dashboard'),
  
  getUsers: (params?: { page?: number; search?: string }) =>
    apiClient.get('/admin/users', { params }),
  
  getUser: (id: number) =>
    apiClient.get(`/admin/users/${id}`),
  
  updateUserStatus: (id: number, isActive: boolean) =>
    apiClient.patch(`/admin/users/${id}/status`, { isActive }),
  
  resetUserPassword: (id: number) =>
    apiClient.post(`/admin/users/${id}/reset-password`),
  
  deleteUser: (id: number) =>
    apiClient.delete(`/admin/users/${id}`),
  
  getDevices: (params?: { page?: number; status?: string }) =>
    apiClient.get('/admin/devices', { params }),
  
  getDevice: (id: number) =>
    apiClient.get(`/admin/devices/${id}`),
  
  forceUnbindDevice: (id: number) =>
    apiClient.post(`/admin/devices/${id}/unbind`),
  
  getOtaPackages: () =>
    apiClient.get('/admin/ota'),
  
  publishOta: (data: Partial<OtaPackage>) =>
    apiClient.post('/admin/ota', data),
}
