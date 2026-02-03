import { onMounted, onUnmounted } from 'vue'
import { io, Socket } from 'socket.io-client'
import { useDevicesStore } from '@/stores/devices'
import { showNotify } from 'vant'

let socket: Socket | null = null

export function useWebSocket() {
    const devicesStore = useDevicesStore()

    onMounted(() => {
        const wsUrl = import.meta.env.VITE_WS_URL || window.location.origin
        socket = io(wsUrl, {
            auth: {
                token: localStorage.getItem('access_token'),
            },
            transports: ['websocket', 'polling'],
        })

        socket.on('connect', () => {
            console.log('✅ WebSocket connected')
        })

        socket.on('disconnect', () => {
            console.log('❌ WebSocket disconnected')
        })

        socket.on('connect_error', (error) => {
            console.error('WebSocket connection error:', error)
        })

        // 设备状态更新
        socket.on('device_status', (data) => {
            console.log('📡 Device status update:', data)
            devicesStore.updateDeviceStatus(data.deviceId, data)
        })

        // Ghost检测通知
        socket.on('ghost_detected', (data) => {
            if (data.level === 'confirmed') {
                showNotify({
                    type: 'warning',
                    message: `🎮 ${data.deviceName} 被物理遥控器控制`,
                    duration: 3000,
                })
                if (data.newState) {
                    devicesStore.updateDeviceStatus(data.deviceId, data.newState)
                }
            } else if (data.level === 'suspected') {
                showNotify({
                    type: 'warning',
                    message: `⚠️ 检测到${data.deviceName}的提示音，建议刷新状态`,
                    duration: 5000,
                })
            }
        })

        // 定时任务执行通知
        socket.on('schedule_executed', (data) => {
            showNotify({
                type: 'success',
                message: `⏰ 定时任务"${data.scheduleName}"已执行`,
                duration: 3000,
            })
        })

        // Routine执行通知
        socket.on('routine_triggered', (data) => {
            showNotify({
                type: 'success',
                message: `🤖 自动化"${data.routineName}"已触发`,
                duration: 3000,
            })
        })

        // 设备上线通知
        socket.on('device_online', (data) => {
            showNotify({
                type: 'success',
                message: `✅ ${data.deviceName} 已上线`,
                duration: 2000,
            })
        })

        // 设备离线通知
        socket.on('device_offline', (data) => {
            showNotify({
                type: 'danger',
                message: `❌ ${data.deviceName} 已离线`,
                duration: 3000,
            })
        })
    })

    onUnmounted(() => {
        if (socket) {
            socket.disconnect()
            socket = null
        }
    })

    return {
        socket,
    }
}
