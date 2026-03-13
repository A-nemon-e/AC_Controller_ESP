import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { 
    AdminStats, 
    UserWithDevices, 
    DeviceWithUser, 
    OtaPackage, 
    OtaUpdate,
    SystemLog 
} from '@/types/admin'
import type { User } from '@/types/user'
import { adminApi } from '@/api/admin'

export const useAdminStore = defineStore('admin', () => {
    // State
    const stats = ref<AdminStats>({
        totalUsers: 0,
        totalDevices: 0,
        onlineDevices: 0,
        totalCommands: 0,
        activeToday: 0,
        activeUsersToday: 0,
        newUsersToday: 0,
        newDevicesToday: 0,
    })

    const users = ref<UserWithDevices[]>([])
    const devices = ref<DeviceWithUser[]>([])
    const otaPackages = ref<OtaPackage[]>([])
    const otaUpdates = ref<OtaUpdate[]>([])
    const systemLogs = ref<SystemLog[]>([])
    const loading = ref(false)
    const currentUser = ref<User | null>(null)

    // Getters
    const userCount = computed(() => users.value.length)
    const deviceCount = computed(() => devices.value.length)
    const onlineDeviceCount = computed(() => devices.value.filter(d => d.isOnline === true).length)
    
    const pendingOtaUpdates = computed(() => 
        otaUpdates.value.filter(u => ['pending', 'downloading', 'installing'].includes(u.status))
    )

    const recentLogs = computed(() => 
        systemLogs.value.slice(0, 100)
    )

    const errorLogs = computed(() => 
        systemLogs.value.filter(log => log.level === 'error')
    )

    // Actions
    const fetchStats = async () => {
        loading.value = true
        try {
            const response = await adminApi.getStats()
            if (response.data) {
                stats.value = response.data
            }
        } catch (error) {
            console.error('Failed to fetch admin stats:', error)
        } finally {
            loading.value = false
        }
    }

    const fetchUsers = async (params?: { page?: number; search?: string }) => {
        loading.value = true
        try {
            const response = await adminApi.getUsers(params)
            if (response.data) {
                users.value = response.data.users || response.data
            }
        } catch (error) {
            console.error('Failed to fetch users:', error)
        } finally {
            loading.value = false
        }
    }

    const fetchDevices = async (params?: { page?: number; status?: string }) => {
        loading.value = true
        try {
            const response = await adminApi.getDevices(params)
            if (response.data) {
                devices.value = response.data.devices || response.data
            }
        } catch (error) {
            console.error('Failed to fetch devices:', error)
        } finally {
            loading.value = false
        }
    }

    const createUser = async (userData: { username: string; password: string; role: string }) => {
        loading.value = true
        try {
            // TODO: 创建用户API
            console.log('Creating user:', userData)
            await fetchUsers()
        } finally {
            loading.value = false
        }
    }

    const updateUser = async (id: number, userData: Partial<User>) => {
        loading.value = true
        try {
            // TODO: 更新用户API
            console.log('Updating user:', id, userData)
            await fetchUsers()
        } finally {
            loading.value = false
        }
    }

    const deleteUser = async (id: number) => {
        loading.value = true
        try {
            await adminApi.deleteUser(id)
            await fetchUsers()
        } catch (error) {
            console.error('Failed to delete user:', error)
            throw error
        } finally {
            loading.value = false
        }
    }

    const fetchOtaPackages = async () => {
        loading.value = true
        try {
            const response = await adminApi.getOtaPackages()
            if (response.data) {
                otaPackages.value = response.data
            }
        } catch (error) {
            console.error('Failed to fetch OTA packages:', error)
        } finally {
            loading.value = false
        }
    }

    const createOtaPackage = async (packageData: Partial<OtaPackage>) => {
        loading.value = true
        try {
            await adminApi.publishOta(packageData)
            await fetchOtaPackages()
        } catch (error) {
            console.error('Failed to create OTA package:', error)
            throw error
        } finally {
            loading.value = false
        }
    }

    const fetchSystemLogs = async (filters?: { level?: string; component?: string; startTime?: string; endTime?: string }) => {
        loading.value = true
        try {
            // TODO: 从API获取系统日志
            console.log('Fetching system logs with filters:', filters)
            
            // 模拟数据
            systemLogs.value = [
                { id: '1', timestamp: new Date().toISOString(), level: 'info', component: 'device', message: '设备 ESP_001 已上线' },
                { id: '2', timestamp: new Date().toISOString(), level: 'error', component: 'ota', message: 'OTA更新失败: ESP_002' },
            ]
        } finally {
            loading.value = false
        }
    }

    const clearSystemLogs = async () => {
        loading.value = true
        try {
            // TODO: 清除系统日志API
            systemLogs.value = []
        } finally {
            loading.value = false
        }
    }

    const exportSystemLogs = async (_format: 'json' | 'csv' = 'json'): Promise<Blob> => {
        // TODO: 导出系统日志
        return new Blob([JSON.stringify(systemLogs.value, null, 2)], { type: 'application/json' })
    }

    const fetchOtaUpdates = async () => {
        loading.value = true
        try {
            // TODO: 从API获取OTA更新列表
        } finally {
            loading.value = false
        }
    }

    return {
        // State
        stats,
        users,
        devices,
        otaPackages,
        otaUpdates,
        systemLogs,
        loading,
        currentUser,
        // Getters
        userCount,
        deviceCount,
        onlineDeviceCount,
        pendingOtaUpdates,
        recentLogs,
        errorLogs,
        // Actions
        fetchStats,
        fetchUsers,
        fetchDevices,
        createUser,
        updateUser,
        deleteUser,
        fetchOtaPackages,
        createOtaPackage,
        fetchSystemLogs,
        clearSystemLogs,
        exportSystemLogs,
        fetchOtaUpdates,
    }
})
