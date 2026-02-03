import dayjs from 'dayjs'

/**
 * 格式化时间
 */
export const formatTime = (hour: number, minute: number): string => {
    return `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`
}

/**
 * 格式化日期时间
 */
export const formatDateTime = (date: string | Date): string => {
    return dayjs(date).format('YYYY-MM-DD HH:mm:ss')
}

/**
 * 格式化日期
 */
export const formatDate = (date: string | Date): string => {
    return dayjs(date).format('YYYY-MM-DD')
}

/**
 * 获取相对时间
 */
export const getRelativeTime = (date: string | Date): string => {
    const now = dayjs()
    const target = dayjs(date)
    const diffDays = now.diff(target, 'day')

    if (diffDays === 0) return '今天'
    if (diffDays === 1) return '昨天'
    if (diffDays === -1) return '明天'
    if (diffDays > 1 && diffDays < 7) return `${diffDays}天前`
    if (diffDays < -1 && diffDays > -7) return `${-diffDays}天后`
    return formatDate(date)
}

/**
 * 模式文本
 */
export const getModeText = (mode: string): string => {
    const modeMap = {
        cool: '制冷',
        heat: '制热',
        fan: '送风',
        dry: '除湿',
        auto: '自动',
    }
    return modeMap[mode] || mode
}

/**
 * 风速文本
 */
export const getFanText = (fan: string): string => {
    const fanMap = {
        auto: '自动',
        low: '低',
        mid: '中',
        high: '高',
        turbo: '强力',
    }
    return fanMap[fan] || fan
}

/**
 * 重复模式文本
 */
export const getRepeatText = (repeat: string): string => {
    const repeatMap = {
        daily: '每天',
        weekdays: '工作日',
        weekends: '周末',
        custom: '自定义',
    }
    return repeatMap[repeat] || repeat
}
