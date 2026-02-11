import { Injectable, Logger } from '@nestjs/common';
import { OnEvent } from '@nestjs/event-emitter';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Device } from './device.entity';
import { MqttService } from '../mqtt/mqtt.service';

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

    constructor(
        @InjectRepository(Device)
        private devicesRepository: Repository<Device>,
        private mqttService: MqttService,
    ) { }

    /**
     * 监听MQTT消息，处理设备发现
     */
    @OnEvent('mqtt.message')
    async handleMqttMessage(event: { topic: string; payload: Buffer }) {
        const { topic, payload } = event;

        // 监听 ac/discovery/# 主题
        if (topic.startsWith('ac/discovery/')) {
            this.logger.log(`[Discovery] 收到发现消息 Topic: ${topic}`);
            const payloadStr = payload.toString().trim(); // 去除可能的空白字符
            this.logger.log(`[Discovery] Payload: ${payloadStr}`);

            try {
                const data = JSON.parse(payloadStr);

                // 验证必需字段
                if (!data.uuid || !data.mac) {
                    this.logger.warn(`[Discovery] ❌ 无效消息 (缺uuid/mac): ${payloadStr}`);
                    return;
                }

                // 只记录未绑定的设备（userId == 0）
                // 注意：有些固件可能发 "0" 字符串，做兼容处理
                const uid = parseInt(data.userId);
                if (!data.userId || uid === 0) {
                    // ✅ 方案A：先查询数据库，检查设备是否已绑定
                    const existingDevice = await this.devicesRepository.findOne({
                        where: { uuid: data.uuid }
                    });

                    if (existingDevice) {
                        // 设备已绑定，自动推送配置恢复
                        this.logger.warn(
                            `[Discovery] ⚠️ 设备 ${data.uuid} 已绑定到用户 ${existingDevice.userId}，但发送了 userId=0。执行自动恢复...`
                        );

                        // 推送配置到 ESP
                        await this.pushRecoveryConfig(existingDevice, data.mac);

                        // ❌ 不加入未绑定设备列表
                        this.logger.log(
                            `[Discovery] ✅ 已推送恢复配置到设备 ${data.uuid}，跳过添加到发现列表`
                        );
                    } else {
                        // 设备未绑定，正常添加到发现列表
                        const deviceInfo: DiscoveredDevice = {
                            uuid: data.uuid,
                            mac: data.mac,
                            ip: data.ip || 'unknown',
                            userId: 0,
                            brand: data.brand || '',
                            model: data.model || 0,
                            timestamp: data.timestamp || Date.now(),
                            lastSeen: new Date(),
                        };

                        this.discoveredDevices.set(data.uuid, deviceInfo);

                        this.logger.log(
                            `[Discovery] ✅ 已添加未绑定设备: ${data.uuid} (${data.mac}) IP: ${data.ip}`
                        );
                    }
                } else {
                    // 如果设备已绑定，从未绑定列表中移除
                    if (this.discoveredDevices.has(data.uuid)) {
                        this.discoveredDevices.delete(data.uuid);
                        this.logger.log(`[Discovery] ℹ️ 设备 ${data.uuid} 已绑定 (ID: ${data.userId})，移除`);
                    } else {
                        this.logger.log(`[Discovery] ℹ️ 忽略已绑定设备: ${data.uuid} (ID: ${data.userId})`);
                    }
                }
            } catch (error) {
                this.logger.error(`[Discovery] ❌ 解析失败: ${error.message} \nPayload Hex: ${payload.toString('hex')}`);
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

        this.logger.log(`[Discovery] API请求获取设备列表。当前缓存总数: ${devices.length}, maxAge: ${maxAgeMinutes}min`);

        // 过滤掉超过指定时间没有更新的设备
        const validDevices = devices.filter((device) => {
            const age = now - device.lastSeen.getTime();
            const isValid = age < maxAge;
            this.logger.debug(`[Discovery] 检查设备 ${device.uuid}: age=${age}ms, valid=${isValid} (lastSeen: ${device.lastSeen.toISOString()})`);
            return isValid;
        });

        this.logger.log(`[Discovery] 过滤后剩余设备数: ${validDevices.length}`);
        return validDevices;
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
     * ✅ 新增：推送恢复配置到重置的ESP设备
     * @param device 数据库中的设备记录
     * @param mac 设备MAC地址（用于备用topic）
     */
    private async pushRecoveryConfig(device: Device, mac: string) {
        const payload = {
            userId: device.userId,
            deviceId: device.id,
            timestamp: Date.now(),
        };

        const payloadStr = JSON.stringify(payload);

        // 策略1: 推送到未绑定状态的标准 topic
        const topicUnbound = `ac/user_0/dev_${device.uuid}/config/update`;
        this.mqttService.publish(topicUnbound, payloadStr);
        this.logger.log(`[Recovery] 推送配置到 ${topicUnbound}`);

        // 策略2: 同时推送到 MAC 地址 topic (确保能收到)
        if (mac) {
            const macClean = mac.replace(/:/g, '');
            const topicMac = `ac/config/${macClean}`;
            this.mqttService.publish(topicMac, payloadStr);
            this.logger.log(`[Recovery] 推送配置到 ${topicMac}`);
        }

        // 策略3: 如果有品牌配置，也推送品牌信息
        if (device.brandConfig) {
            try {
                const brandCfg = JSON.parse(device.brandConfig);
                const brandPayload = {
                    brand: brandCfg.brandId,
                    model: brandCfg.model || 1,
                };
                const brandPayloadStr = JSON.stringify(brandPayload);

                const topicConfig = `ac/config/${mac.replace(/:/g, '')}`;
                this.mqttService.publish(topicConfig, brandPayloadStr);
                this.logger.log(`[Recovery] 推送品牌配置到 ${topicConfig}: ${brandPayloadStr}`);
            } catch (e) {
                this.logger.warn(`[Recovery] 解析 brandConfig 失败: ${e.message}`);
            }
        }
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
