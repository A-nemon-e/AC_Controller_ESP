import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { devicesApi } from '@/api/devices'
import type { Device, DeviceState } from '@/types/device'

export const useDevicesStore = defineStore('devices', () => {
    const devices = ref<Device[]>([])
    const currentDevice = ref<Device | null>(null)
    const loading = ref(false)

    const fetchDevices = async () => {
        loading.value = true
        try {
            devices.value = await devicesApi.getAll()
            // 自动选择第一个设备
            if (devices.value.length > 0 && !currentDevice.value) {
                currentDevice.value = devices.value[0]
            }
        } catch (error) {
            console.error('Failed to fetch devices:', error)
        } finally {
            loading.value = false
        }
    }

    const setCurrentDevice = (device: Device) => {
        currentDevice.value = device
    }

    const updateDeviceStatus = (deviceId: number, state: Partial<DeviceState>) => {
        const device = devices.value.find((d) => d.id === deviceId)
        if (device && device.lastState) {
            device.lastState = { ...device.lastState, ...state }
        }
        // 如果是当前设备，也更新
        if (currentDevice.value?.id === deviceId && currentDevice.value.lastState) {
            currentDevice.value.lastState = { ...currentDevice.value.lastState, ...state }
        }
    }

    const addDevice = (device: Device) => {
        devices.value.push(device)
    }

    const removeDevice = (deviceId: number) => {
        const index = devices.value.findIndex((d) => d.id === deviceId)
        if (index !== -1) {
            devices.value.splice(index, 1)
        }
        if (currentDevice.value?.id === deviceId) {
            currentDevice.value = devices.value[0] || null
        }
    }

    return {
        devices,
        currentDevice,
        loading,
        fetchDevices,
        setCurrentDevice,
        updateDeviceStatus,
        addDevice,
        removeDevice,
    }
})
