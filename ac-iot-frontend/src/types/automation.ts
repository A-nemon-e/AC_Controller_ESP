// 定时任务类型
export interface Schedule {
    id: number
    name: string
    deviceId: number
    enabled: boolean
    hour: number
    minute: number
    repeat: 'daily' | 'weekdays' | 'weekends' | 'custom'
    weekdays?: number[]
    action: {
        power: boolean
        mode?: string
        temp?: number
        fan?: string
    }
    lastRun: string | null
    userId: number
}

// Routine自动化类型
export interface Routine {
    id: number
    name: string
    deviceId: number
    enabled: boolean
    logic: 'AND' | 'OR'
    triggers: Trigger[]
    action: {
        power: boolean
        mode?: string
        temp?: number
        fan?: string
    }
    lastTriggered: string | null
    userId: number
}

export interface Trigger {
    type: 'temp' | 'hum' | 'time' | 'weekday' | 'date' | 'power' | 'mode' | 'current'
    operator: '>' | '<' | '=' | '>=' | '<='
    value: number | string | boolean
}
