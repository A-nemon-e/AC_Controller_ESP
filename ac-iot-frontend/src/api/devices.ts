import apiClient from './client'
import type { Device, DeviceState, DeviceConfig, DiscoveredDevice, Brand } from '@/types/device'

export const devicesApi = {
    // 获取所有设备
    getAll: () => apiClient.get<Device[]>('/devices'),

    // 获取单个设备
    getById: (id: number) => apiClient.get<Device>(`/devices/${id}`),

    // 添加设备
    create: (data: { uuid: string; name: string }) =>
        apiClient.post<Device>('/devices', data),

    // 删除设备
    delete: (id: number) => apiClient.delete(`/devices/${id}`),

    // 发送控制命令
    sendCommand: (id: number, command: Partial<DeviceState>) =>
        apiClient.post(`/devices/${id}/cmd`, command),

    // 获取设备配置
    getConfig: (id: number) => apiClient.get<DeviceConfig>(`/devices/${id}/config`),

    // 更新设备配置
    updateConfig: (id: number, config: Partial<DeviceConfig>) =>
        apiClient.patch(`/devices/${id}/config`, config),

    // 设备发现
    getDiscoveredDevices: (maxAge?: number) =>
        apiClient.get<{ devices: DiscoveredDevice[]; count: number }>(
            `/devices/discovery/available${maxAge ? `?maxAge=${maxAge}` : ''}`
        ),

    // 获取品牌列表
    getBrands: () => apiClient.get<{ brands: Brand[] }>('/devices/brands'),

    // 设置品牌
    setBrand: (id: number, brandId: string, model: number) =>
        apiClient.post(`/devices/${id}/setup/brand`, { brandId, model }),

    // 自动检测
    startAutoDetect: (id: number) =>
        apiClient.post(`/devices/${id}/auto-detect/start`),

    stopAutoDetect: (id: number) =>
        apiClient.post(`/devices/${id}/auto-detect/stop`),

    getAutoDetectStatus: (id: number) =>
        apiClient.get(`/devices/${id}/auto-detect/status`),
}
