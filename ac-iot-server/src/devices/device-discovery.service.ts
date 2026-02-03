import { Injectable, Logger } from '@nestjs/common';
import { OnEvent } from '@nestjs/event-emitter';

export interface DiscoveredDevice {
    uuid: string;
    mac: string;
    ip: string;
    userId: number;
    brand: string;
    model: number;
    timestamp: number;
    lastSeen: Date;
}

@Injectable()
export class DeviceDiscoveryService {
    private readonly logger = new Logger(DeviceDiscoveryService.name);
    private discoveredDevices: Map<string, DiscoveredDevice> = new Map();

    /**
     * 监听MQTT消息，处理设备发现
     */
    @OnEvent('mqtt.message')
    async handleMqttMessage(event: { topic: string; payload: Buffer }) {
        const { topic, payload } = event;

        // 监听 ac/discovery/# 主题
        if (topic.startsWith('ac/discovery/')) {
            try {
                const data = JSON.parse(payload.toString());

                // 验证必需字段
                if (!data.uuid || !data.mac) {
                    this.logger.warn(`无效的设备发现消息: ${payload.toString()}`);
                    return;
                }

                // 只记录未绑定的设备（userId == 0）
                if (data.userId === 0 || !data.userId) {
                    const deviceInfo: DiscoveredDevice = {
                        uuid: data.uuid,
                        mac: data.mac,
                        ip: data.ip || 'unknown',
                        userId: data.userId || 0,
                        brand: data.brand || '',
                        model: data.model || 0,
                        timestamp: data.timestamp || Date.now(),
                        lastSeen: new Date(),
                    };

                    this.discoveredDevices.set(data.uuid, deviceInfo);

                    this.logger.log(
                        `发现新设备: ${data.uuid} (${data.mac}) IP: ${data.ip}`
                    );
                } else {
                    // 如果设备已绑定，从未绑定列表中移除
                    if (this.discoveredDevices.has(data.uuid)) {
                        this.discoveredDevices.delete(data.uuid);
                        this.logger.log(`设备 ${data.uuid} 已绑定，从发现列表移除`);
                    }
                }
            } catch (error) {
                this.logger.error('解析设备发现消息失败', error.stack);
            }
        }
    }

    /**
     * 获取所有可用设备
     * @param maxAgeMinutes 设备最后在线时间（分钟），默认5分钟
     */
    getAvailableDevices(maxAgeMinutes: number = 5): DiscoveredDevice[] {
        const now = Date.now();
        const maxAge = maxAgeMinutes * 60 * 1000;
        const devices = Array.from(this.discoveredDevices.values());

        // 过滤掉超过指定时间没有更新的设备
        return devices.filter(
            (device) => now - device.lastSeen.getTime() < maxAge
        );
    }

    /**
     * 清理离线设备（定时任务）
     * @param maxAgeMinutes 设备离线时间（分钟），默认10分钟
     */
    cleanupOfflineDevices(maxAgeMinutes: number = 10) {
        const now = Date.now();
        const maxAge = maxAgeMinutes * 60 * 1000;
        let cleanedCount = 0;

        for (const [uuid, device] of this.discoveredDevices.entries()) {
            if (now - device.lastSeen.getTime() > maxAge) {
                this.discoveredDevices.delete(uuid);
                cleanedCount++;
                this.logger.debug(`设备离线已清理: ${uuid}`);
            }
        }

        if (cleanedCount > 0) {
            this.logger.log(`清理了 ${cleanedCount} 个离线设备`);
        }

        return cleanedCount;
    }

    /**
     * 获取发现的设备数量
     */
    getDiscoveredDeviceCount(): number {
        return this.discoveredDevices.size;
    }

    /**
     * 根据UUID获取设备
     */
    getDeviceByUuid(uuid: string): DiscoveredDevice | undefined {
        return this.discoveredDevices.get(uuid);
    }

    /**
     * 移除指定设备（当设备被绑定后调用）
     */
    removeDevice(uuid: string): boolean {
        const existed = this.discoveredDevices.has(uuid);
        if (existed) {
            this.discoveredDevices.delete(uuid);
            this.logger.log(`设备 ${uuid} 已从发现列表移除`);
        }
        return existed;
    }

    /**
     * 清空所有设备（用于测试）
     */
    clearAll() {
        const count = this.discoveredDevices.size;
        this.discoveredDevices.clear();
        this.logger.log(`已清空所有设备 (${count}个)`);
    }
}
