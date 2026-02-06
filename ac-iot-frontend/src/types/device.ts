// 设备相关类型
export interface Device {
    id: number
    name: string
    uuid: string
    brandId: string
    model: number
    userId: number
    config: DeviceConfig
    lastState: DeviceState | null
    isOnline?: boolean
    lastSeen?: string
}

export interface DeviceConfig {
    enableCurrent: boolean
    tempUnit: 'C' | 'F'
    beepDetection: {
        enabled: boolean
        openDuration: number
        closeDuration: number
        detectionWindow: number
        noiseFilter: boolean
        minInterval: number
    }
}

export interface DeviceState {
    power: boolean
    mode: 'cool' | 'heat' | 'fan' | 'dry' | 'auto'
    setTemp: number
    fan: 'auto' | 'low' | 'mid' | 'high' | 'turbo'
    swingVertical: boolean
    swingHorizontal: boolean
    turbo?: boolean
    quiet?: boolean
    light?: boolean
    temp?: number
    hum?: number
    current?: number
    adcRaw?: number
    source: 'api' | 'ir_recv' | 'routine' | 'schedule'
    timestamp: string
}

export interface DiscoveredDevice {
    uuid: string
    mac: string
    ip: string
    userId: number
    brand: string
    model: number
    timestamp: number
    lastSeen: string
}

export interface Brand {
    id: string
    name: string
    models: number[]
}
