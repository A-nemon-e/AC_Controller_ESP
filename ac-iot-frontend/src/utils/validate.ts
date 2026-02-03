/**
 * 验证UUID格式
 */
export const isValidUUID = (uuid: string): boolean => {
    const pattern = /^ESP_[A-Z0-9]{12}$/
    return pattern.test(uuid)
}

/**
 * 验证温度范围
 */
export const isValidTemp = (temp: number): boolean => {
    return temp >= 16 && temp <= 30
}

/**
 * 验证IP地址
 */
export const isValidIP = (ip: string): boolean => {
    const pattern = /^(\d{1,3}\.){3}\d{1,3}$/
    return pattern.test(ip)
}

/**
 * 验证MAC地址
 */
export const isValidMAC = (mac: string): boolean => {
    const pattern = /^([0-9A-F]{2}:){5}[0-9A-F]{2}$/i
    return pattern.test(mac)
}
