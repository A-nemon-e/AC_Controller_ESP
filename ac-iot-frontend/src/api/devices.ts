import apiClient from './client'
import type { CreateDeviceDto, Device, DiscoveredDevice, DisplayConfig, CommandPayload } from '@/types/device'

export const devicesApi = {
    getAll: async (): Promise<Device[]> => {
        const response = await apiClient.get('/devices')
        return response.data
    },
    
    getById: async (id: number): Promise<Device> => {
        const response = await apiClient.get(`/devices/${id}`)
        return response.data
    },
    
    create: async (device: CreateDeviceDto): Promise<Device> => {
        const response = await apiClient.post('/devices', device)
        return response.data
    },
    
    delete: async (id: number) => {
        await apiClient.delete(`/devices/${id}`)
    },
    
    getDiscoveredDevices: async (): Promise<{ devices: DiscoveredDevice[] }> => {
        const response = await apiClient.get('/devices/discover')
        return response.data
    },
    
    // 获取显示配置
    getDisplayConfig: async (id: number): Promise<DisplayConfig> => {
        const response = await apiClient.get(`/devices/${id}/display-config`)
        return response.data
    },
    
    // 更新显示配置
    updateDisplayConfig: async (id: number, config: DisplayConfig): Promise<DisplayConfig> => {
        const response = await apiClient.patch(`/devices/${id}/display-config`, config)
        return response.data
    },
    
    // 发送命令
    sendCommand: async (id: number, command: CommandPayload): Promise<any> => {
        const response = await apiClient.post(`/devices/${id}/cmd`, command)
        return response.data
    },
}
