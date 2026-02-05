import { Injectable, NotFoundException, ForbiddenException, Logger } from '@nestjs/common';
import { OnEvent } from '@nestjs/event-emitter';
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
        let device = await this.devicesRepository.findOne({ where: { uuid: createDeviceDto.uuid } });

        if (device) {
            this.logger.log(`Device ${createDeviceDto.uuid} already exists, updating ownership (Re-claim)`);
            // 更新所有权
            device.userId = userId;
            device.name = createDeviceDto.name; // 更新名字
        } else {
            device = this.devicesRepository.create({
                ...createDeviceDto,
                userId,
            });
        }

        const saved = await this.devicesRepository.save(device);

        // ✅ 推送配置到ESP
        // 1. 标准推送到 ac/user_0/dev_{uuid}/config/update (针对新设备)
        await this.pushDeviceConfig(saved);

        // 2. ✅ 额外：如果有MAC地址，推送到 ac/config/{mac} (针对重置或重连设备)
        if (createDeviceDto.mac) {
            const macClean = createDeviceDto.mac.replace(/:/g, ''); // 确保无冒号
            const topicMac = `ac/config/${macClean}`;
            const payload = {
                userId: saved.userId,
                deviceId: saved.id,
                // 这里我们不需要 "update": true 之类的，因为 config_manager.cpp 不检查这个
                // 它只要看到 json 里有 userId 就会更新
            };
            this.mqttService.publish(topicMac, JSON.stringify(payload));
            this.logger.log(`Forced config update to MAC topic ${topicMac}`);
        }

        return saved;
    }

    async findAll(userId: number): Promise<Device[]> {
        return this.devicesRepository.find({ where: { userId } });
    }

    async findOneById(id: number): Promise<Device | null> {
        return this.devicesRepository.findOne({ where: { id } });
    }

    async findOne(userId: number, id: number): Promise<Device> {
        const device = await this.devicesRepository.findOne({ where: { id } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');
        return device;
    }

    async remove(userId: number, id: number): Promise<void> {
        const device = await this.devicesRepository.findOne({ where: { id } });
        if (!device) {
            throw new NotFoundException('Device not found');
        }
        if (device.userId !== userId) {
            throw new ForbiddenException('Not your device');
        }

        // ✅ 发送解绑命令 (重置 userId 为 0)
        // 这样设备会收到配置并更新 EEPROM，然后重启或重发 Discovery
        const topic = `ac/user_${userId}/dev_${device.uuid}/config/update`;
        const payload = JSON.stringify({ userId: 0, deviceId: 0 });

        this.mqttService.publish(topic, payload);
        this.logger.log(`Unbinding device ${device.uuid}: sent config reset to ${topic}`);

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
            model: model ? parseInt(model) : 1 // ✅ Default Model = 1
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

    // 暂存自动检测状态 (Memory Cache)
    // Key: deviceId, Value: { status: 'idle'|'detecting'|'success'|'fail', result?: any, timestamp: number }
    private autoDetectStates = new Map<number, any>();

    // 暂存学习结果 (Memory Cache)
    // Key: deviceId, Value: { status: 'learning'|'success'|'timeout', key: string, raw?: string, timestamp: number }
    private learningStates = new Map<number, any>();

    /**
     * ✅ MQTT 消息监听 (处理 brands/list, auto_detect, learn)
     */
    @OnEvent('mqtt.message')
    async handleMqttMessage(event: { topic: string, payload: any }) {
        const { topic, payload } = event;
        const msgString = payload.toString();

        try {
            // 解析 UUID (假设 topic结构固定: ac/user_X/dev_UUID/...)
            const parts = topic.split('/');
            const uuidPart = parts.find(p => p.startsWith('dev_'));
            if (!uuidPart) return;
            const uuid = uuidPart.replace('dev_', '');

            // 查找对应的 Device ID (为了更新 memory cache)
            // 注意：频繁查库可能影响性能，这只是一个简单实现。
            // 生产环境可以维护 uuid -> id 的映射
            const device = await this.devicesRepository.findOne({ where: { uuid } });
            if (!device) return;

            // 1. 处理 brands/list 响应
            if (topic.endsWith('/brands/list')) {
                const data = JSON.parse(msgString);
                let brands: string[] = [];
                if (Array.isArray(data)) {
                    brands = data;
                } else if (data.protocols) {
                    brands = data.protocols;
                } else if (data.brands) {
                    brands = data.brands;
                }

                if (brands.length > 0) {
                    device.supportedBrands = brands;
                    await this.devicesRepository.save(device);
                    this.logger.log(`Updated supportedBrands for ${uuid}: ${brands.length} protocols`);
                }
            }
            // 2. 处理 auto_detect 状态/结果
            else if (topic.includes('/auto_detect/')) {
                const data = JSON.parse(msgString);

                if (topic.endsWith('/status')) {
                    // Update Status
                    const current = this.autoDetectStates.get(device.id) || {};
                    this.autoDetectStates.set(device.id, {
                        ...current,
                        status: data.status, // detecting, idle
                        message: data.message,
                        timestamp: Date.now()
                    });
                } else if (topic.endsWith('/result')) {
                    // Update Result
                    this.autoDetectStates.set(device.id, {
                        status: data.success ? 'success' : 'fail',
                        result: data,
                        timestamp: Date.now()
                    });
                    this.logger.log(`Auto Detect Result for ${uuid}: ${data.success}`);
                }
            }
            // 3. 处理 learn/result
            else if (topic.endsWith('/learn/result')) {
                const data = JSON.parse(msgString);
                // Schema: { key, raw, success, error? }
                this.learningStates.set(device.id, {
                    status: data.success ? 'success' : 'timeout',
                    key: data.key,
                    raw: data.raw,
                    timestamp: Date.now()
                });
                this.logger.log(`Learn Result for ${uuid} (key=${data.key}): ${data.success}`);
            }
            // 4. ✅ 处理 status 状态上报 (Fixing Sync Issue)
            else if (topic.endsWith('/status')) {
                const data = JSON.parse(msgString);
                // data: { power, mode, targetTemp, fan, swingV, swingH, temp, hum, current, ... }

                // Update lastState
                device.lastState = {
                    ...device.lastState, // Keep existing fields
                    power: data.power,
                    mode: data.mode,
                    setTemp: data.setTemp || data.targetTemp || device.lastState?.setTemp || 26, // Robust fallback
                    fan: data.fan !== undefined ? (typeof data.fan === 'number' ? (data.fan === 0 ? 'auto' : 'low') : data.fan) : device.lastState?.fan, // Simple fan mapping fallback
                    swingVertical: data.swingVertical,
                    swingHorizontal: data.swingHorizontal,
                    temp: data.temp,
                    hum: data.hum,
                    current: data.current
                };

                // Fan mapping enhancement if ESP sends numbers (0=auto, 1=low, 2=mid, 3=high)
                if (typeof data.fan === 'number') {
                    const fanMap = ['auto', 'low', 'mid', 'high'];
                    if (data.fan >= 0 && data.fan < fanMap.length) {
                        device.lastState.fan = fanMap[data.fan];
                    }
                }

                await this.devicesRepository.save(device);
                // Log only occasionally or debug
                // this.logger.log(`Updated status for ${uuid}: ${JSON.stringify(device.lastState)}`);
            }

        } catch (e) {
            this.logger.error(`Error processing MQTT message ${topic}: ${e.message}`);
        }
    }

    /**
     * 获取自动检测状态 (Real Implementation)
     */
    async getAutoDetectStatus(deviceId: number) {
        const state = this.autoDetectStates.get(deviceId);

        // 如果没有状态，默认返回 idle
        if (!state) {
            return {
                deviceId,
                status: 'idle',
                timestamp: Date.now()
            };
        }

        // 如果状态太旧 (比如超过 2 分钟)，重置为 idle
        if (Date.now() - state.timestamp > 120000) {
            this.autoDetectStates.delete(deviceId);
            return {
                deviceId,
                status: 'idle',
                timestamp: Date.now()
            };
        }

        return {
            deviceId,
            ...state
        };
    }

    /**
     * 获取设备支持的品牌列表 (Dynamic)
     */
    async getSupportedBrands(userId: number, deviceId: number) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        // 如果已有缓存的品牌列表，且不为空，直接返回
        if (device.supportedBrands && device.supportedBrands.length > 0) {
            return { status: 'ready', brands: device.supportedBrands };
        }

        // 如果为空，触发一次查询 (MQTT Request)
        const topic = `ac/user_${userId}/dev_${device.uuid}/brands/get`;
        // 防止短时间频繁触发? 简单起见，每次前端轮询都发一次也行，或者加个内存节流。
        // 这里假设前端轮询间隔 1.5s，是可以接受的。
        this.mqttService.publish(topic, JSON.stringify({}));

        return { status: 'loading', brands: [] };
    }

    /**
     * 保存录制的场景 (更新 IR Config)
     */
    async saveScenes(userId: number, deviceId: number, scenes: any[]) {
        const device = await this.devicesRepository.findOne({ where: { id: deviceId } });
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== userId) throw new ForbiddenException('Access denied');

        if (!device.irConfig) device.irConfig = {};

        // Merge scenes
        for (const scene of scenes) {
            // scene: { key: 'cool_26', raw: '...' }
            if (scene.key && scene.raw) {
                device.irConfig[scene.key] = scene.raw;
            }
        }

        // 标记 Setup 状态
        if (Object.keys(device.irConfig).length > 0) {
            device.setupStatus = 'ready';
        }

        await this.devicesRepository.save(device);
        this.logger.log(`Saved ${scenes.length} scenes for device ${deviceId}`);

        return { success: true };
    }

    /**
     * ✅ 新增：获取学习结果 (Polling Interface)
     */
    async getLearningResult(deviceId: number) {
        const state = this.learningStates.get(deviceId);

        if (!state) {
            return { status: 'waiting' };
        }

        // 简短缓存 (1分钟过期)
        if (Date.now() - state.timestamp > 60000) {
            this.learningStates.delete(deviceId);
            return { status: 'waiting' }; // 过期视为无数据
        }

        return state;
    }

    /**
     * ✅ 新增：推送设备配置到MQTT (用于设备绑定)
     * 发送到: ac/user_0/dev_{uuid}/config/update 确保未绑定设备能收到
     */
    private async pushDeviceConfig(device: Device) {
        // ... (保持原样)
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
    }
}
