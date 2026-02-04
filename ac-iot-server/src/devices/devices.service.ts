import { Injectable, NotFoundException, ForbiddenException, Logger } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, MoreThanOrEqual } from 'typeorm';
import { Device } from './device.entity';
import { CreateDeviceDto } from './dto/create-device.dto';
import { AcCommandDto } from './dto/command.dto';
import { MqttService } from '../mqtt/mqtt.service';

import { AuditLog } from './audit-log.entity';
import { SensorReading } from './sensor-reading.entity';

@Injectable()
export class DevicesService {
    private readonly logger = new Logger(DevicesService.name);

    constructor(
        @InjectRepository(Device)
        private devicesRepository: Repository<Device>,
        @InjectRepository(AuditLog)
        private auditLogRepository: Repository<AuditLog>,
        @InjectRepository(SensorReading)
        private sensorReadingRepository: Repository<SensorReading>,
        private mqttService: MqttService,
    ) { }

    async create(userId: number, createDeviceDto: CreateDeviceDto): Promise<Device> {
        const device = this.devicesRepository.create({
            ...createDeviceDto,
            userId,
        });
        const saved = await this.devicesRepository.save(device);

        // ✅ 新增：推送配置到ESP，完成设备绑定
        await this.pushDeviceConfig(saved);

        return saved;
    }

    async findAll(userId: number): Promise<Device[]> {
        return this.devicesRepository.find({ where: { userId } });
    }

    async findOneById(id: number): Promise<Device | null> {
        return this.devicesRepository.findOne({ where: { id } });
    }

    async remove(userId: number, id: number): Promise<void> {
        const device = await this.devicesRepository.findOne({ where: { id } });
        if (!device) {
            throw new NotFoundException('Device not found');
        }
        if (device.userId !== userId) {
            throw new ForbiddenException('Not your device');
        }
        await this.devicesRepository.delete(id);
    }

    async sendCommand(userId: number, deviceId: number, command: AcCommandDto) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });

        if (!device) {
            throw new NotFoundException('Device not found');
        }

        if (device.userId !== userId) {
            throw new ForbiddenException('Access denied');
        }

        // Construct MQTT Topic: ac/user_{uid}/dev_{uuid}/cmd
        const topic = `ac/user_${userId}/dev_${device.uuid}/cmd`;

        let payloadString = JSON.stringify(command);
        let strategy = 'direct'; // 默认直接发送JSON

        // ===== ✅ 优先级1: 品牌协议模式 =====
        if (device.brandConfig) {
            try {
                const brandCfg = JSON.parse(device.brandConfig);
                if (brandCfg.brandId) {
                    // 如果配置了品牌，直接发送语义化命令，ESP会用协议编码
                    this.logger.log(`[Brand Protocol] Device ${deviceId} using brand ${brandCfg.brandId}, sending semantic command`);
                    strategy = 'brand_protocol';
                    // payload不变，保持语义化命令
                }
            } catch (e) {
                this.logger.warn(`Failed to parse brandConfig for device ${deviceId}: ${e.message}`);
            }
        }

        // ===== ⚠️ 优先级2: Raw学习数据（降级） =====
        // 仅当品牌协议未配置或失败时才使用
        if (strategy !== 'brand_protocol' && device.irConfig) {
            let key = '';
            if (command.power === false) {
                key = 'off';
            } else {
                if (command.mode && command.temp) {
                    key = `${command.mode}_${command.temp}`;
                }
            }

            if (key && device.irConfig[key]) {
                const rawData = device.irConfig[key];
                this.logger.log(`[Custom IR] Found learned code for ${key}, sending RAW data`);
                payloadString = JSON.stringify({ ...command, raw: rawData });
                strategy = 'raw';
            }
        }

        this.logger.log(`Sending command to ${topic} (strategy: ${strategy}): ${payloadString}`);
        this.mqttService.publish(topic, payloadString);

        return { success: true, topic, payload: payloadString, strategy };
    }

    async getLogs(
        userId: number,
        deviceId: number,
        limit: number = 50,
        action?: string,
        since?: Date,
    ): Promise<AuditLog[]> {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        const query: any = { device: { id: deviceId } };

        // 按类型筛选
        if (action) {
            query.action = action;
        }

        // 按时间筛选
        if (since) {
            query.timestamp = MoreThanOrEqual(since);
        }

        return this.auditLogRepository.find({
            where: query,
            order: { timestamp: 'DESC' },
            take: limit,
        });
    }

    async getReadings(userId: number, deviceId: number, limit: number = 100): Promise<SensorReading[]> {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        return this.sensorReadingRepository.find({
            where: { device: { id: deviceId } },
            order: { timestamp: 'DESC' },
            take: limit,
        });
    }

    async getConfig(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        return {
            id: device.id,
            name: device.name,
            uuid: device.uuid,
            brandConfig: device.brandConfig,
            micConfig: device.micConfig,
            irConfig: device.irConfig,
            lastState: device.lastState,
        };
    }

    async updateConfig(userId: number, deviceId: number, dto: any) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        // 更新配置
        if (dto.brandConfig !== undefined) device.brandConfig = dto.brandConfig;
        if (dto.micConfig !== undefined) {
            device.micConfig = { ...device.micConfig, ...dto.micConfig };
        }
        if (dto.irConfig !== undefined) {
            device.irConfig = dto.irConfig;
        }
        if (dto.enableCurrent !== undefined) {
            device.enableCurrent = dto.enableCurrent;
        }

        await this.devicesRepository.save(device);
        return device;
    }

    // ===== 设备初始化向导 =====

    async getSetupStatus(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        return {
            setupStatus: device.setupStatus,
            brandConfig: device.brandConfig,
            learnedKeys: device.irConfig ? Object.keys(device.irConfig) : [],
        };
    }

    async setBrand(userId: number, deviceId: number, brandId: string, model?: string) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        // 保存品牌配置
        device.brandConfig = JSON.stringify({ brandId, model });
        device.setupStatus = 'brand_selected';

        await this.devicesRepository.save(device);

        // ✅ 新增：通过MQTT发送品牌配置给ESP
        const topic = `ac/user_${userId}/dev_${device.uuid}/config`;
        this.mqttService.publish(topic, JSON.stringify({
            brand: brandId,
            model: model ? parseInt(model) : 0
        }));

        this.logger.log(`Device ${deviceId} brand set to ${brandId} ${model || ''}, config sent via MQTT`);
        return device;
    }

    async startLearnAll(userId: number, deviceId: number, keys: string[]) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        // 更新状态为学习中
        device.setupStatus = 'learning';
        await this.devicesRepository.save(device);

        this.logger.log(`Device ${deviceId} started learning ${keys.length} keys`);
        return { message: 'Learning started', keys, totalKeys: keys.length };
    }

    async completeSetup(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        device.setupStatus = 'ready';
        await this.devicesRepository.save(device);

        this.logger.log(`Device ${deviceId} setup completed`);
        return { message: 'Setup completed', device };
    }

    // ===== 自动协议检测 =====

    /**
     * 启动自动协议检测
     */
    async startAutoDetect(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });

        if (!device || device.userId !== userId) {
            throw new NotFoundException('Device not found');
        }

        // 发送MQTT命令到ESP
        const topic = `ac/user_${userId}/dev_${device.uuid}/auto_detect`;
        const payload = JSON.stringify({ action: 'start' });

        this.mqttService.publish(topic, payload);

        this.logger.log(`Auto-detect started for device ${deviceId}`);

        return {
            message: '已启动自动检测，请在30秒内按下遥控器任意键',
            timeout: 30,
            deviceId,
            status: 'detecting'
        };
    }

    /**
     * 停止自动协议检测
     */
    async stopAutoDetect(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });

        if (!device || device.userId !== userId) {
            throw new NotFoundException('Device not found');
        }

        const topic = `ac/user_${userId}/dev_${device.uuid}/auto_detect`;
        const payload = JSON.stringify({ action: 'stop' });

        this.mqttService.publish(topic, payload);

        this.logger.log(`Auto-detect stopped for device ${deviceId}`);

        return {
            message: '已停止自动检测',
            deviceId,
            status: 'idle'
        };
    }

    /**
     * 获取自动检测状态
     * 注意：这是一个临时方法，实际状态应该通过WebSocket推送
     */
    async getAutoDetectStatus(deviceId: number) {
        // 简单返回，实际状态通过MQTT uplink和WebSocket维护
        return {
            deviceId,
            status: 'idle',  // 默认空闲状态
            note: '实际状态请通过WebSocket订阅'
        };
    }

    /**
     * ✅ 新增：推送设备配置到MQTT (用于设备绑定)
     * 发送到: ac/user_0/dev_{uuid}/config/update 确保未绑定设备能收到
     */
    private async pushDeviceConfig(device: Device) {
        // 注意：这里我们特意发送到 user_0 的topic，因为未绑定的设备监听的是这个
        // 如果设备已经绑定过（比如改绑），它可能监听的是旧用户的topic
        // 为了稳健性，我们可以同时发送到 user_0 和 user_{current} (如果未来支持)

        // 策略：发送到 user_0 (针对新设备)
        const topicUnbound = `ac/user_0/dev_${device.uuid}/config/update`;

        const payload = {
            userId: device.userId,
            deviceId: device.id,
            timestamp: Date.now(),
        };

        const payloadStr = JSON.stringify(payload);

        // 发送给未绑定状态的Topic
        this.mqttService.publish(topicUnbound, payloadStr);
        this.logger.log(`Pushed bind config to ${topicUnbound}: ${payloadStr}`);

        // 也可以尝试发送到当前用户的Topic (如果设备已经部分配置过)
        // const topicBound = `ac/user_${device.userId}/dev_${device.uuid}/config/update`;
        // this.mqttService.publish(topicBound, payloadStr);
    }
}
