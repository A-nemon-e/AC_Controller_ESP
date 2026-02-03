export const MODES = [
    { value: 'cool', label: '制冷', icon: '❄️' },
    { value: 'heat', label: '制热', icon: '♨️' },
    { value: 'fan', label: '送风', icon: '💨' },
    { value: 'dry', label: '除湿', icon: '💧' },
    { value: 'auto', label: '自动', icon: '🔄' },
] as const

export const FAN_SPEEDS = [
    { value: 'auto', label: '自动' },
    { value: 'low', label: '低' },
    { value: 'mid', label: '中' },
    { value: 'high', label: '高' },
    { value: 'turbo', label: '强力' },
] as const

export const TEMP_RANGE = {
    min: 16,
    max: 30,
    default: 26,
} as const

export const REPEAT_OPTIONS = [
    { text: '每天', value: 'daily' },
    { text: '仅工作日', value: 'weekdays' },
    { text: '仅周末', value: 'weekends' },
    { text: '自定义', value: 'custom' },
] as const

export const TRIGGER_TYPES = [
    { text: '温度', value: 'temp' },
    { text: '湿度', value: 'hum' },
    { text: '时间', value: 'time' },
    { text: '星期', value: 'weekday' },
    { text: '日期', value: 'date' },
    { text: '电源状态', value: 'power' },
    { text: '模式', value: 'mode' },
    { text: '电流', value: 'current' },
] as const

export const OPERATORS = [
    { text: '大于', value: '>' },
    { text: '小于', value: '<' },
    { text: '等于', value: '=' },
    { text: '大于等于', value: '>=' },
    { text: '小于等于', value: '<=' },
] as const
