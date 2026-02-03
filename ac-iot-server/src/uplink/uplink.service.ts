import { Injectable, Logger } from '@nestjs/common';
import { OnEvent } from '@nestjs/event-emitter';
import { EventEmitter2 } from '@nestjs/event-emitter';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Device } from '../devices/device.entity';
import { SensorReading } from '../devices/sensor-reading.entity';
import { AuditLog, AuditAction } from '../devices/audit-log.entity';

import { RoutinesService } from '../routines/routines.service';
import { LearnService } from '../devices/learn.service';

@Injectable()
export class UplinkService {
    private readonly logger = new Logger(UplinkService.name);

    constructor(
        @InjectRepository(Device)
        private deviceRepo: Repository<Device>,
        @InjectRepository(SensorReading)
        private sensorRepo: Repository<SensorReading>,
        @InjectRepository(AuditLog)
        private auditRepo: Repository<AuditLog>,
        private routinesService: RoutinesService,
        private learnService: LearnService,
        private eventEmitter: EventEmitter2,
    ) { }

    @OnEvent('mqtt.message')
    async handleMqttMessage(event: { topic: string, payload: Buffer }) {
        const { topic, payload } = event;
        const message = payload.toString();

        try {
            // Topic structure: ac/user_{uid}/dev_{uuid}/{type}
            // Regex to parse
            const match = topic.match(/^ac\/user_(\d+)\/dev_([a-zA-Z0-9_\-]+)\/(.+)$/);
            if (!match) return; // Not our topic

            const userId = parseInt(match[1]);
            const deviceUuid = match[2];
            const type = match[3];

            this.logger.debug(`Router: Type=${type}, User=${userId}, UUID=${deviceUuid}`);

            const device = await this.deviceRepo.findOne({ where: { uuid: deviceUuid } });
            if (!device) {
                this.logger.warn(`Device not found for UUID: ${deviceUuid}`);
                return;
            }

            const data = JSON.parse(message);

            switch (type) {
                case 'status':
                    await this.handleStatus(device, data);
                    break;
                case 'event':
                    await this.handleEvent(device, data);
                    break;
                case 'learn/result':
                    await this.handleLearn(device, data);
                    break;
                case 'auto_detect/result':  // ✅  新增
                    await this.handleAutoDetectResult(device, data);
                    break;
                case 'auto_detect/status':  // ✅ 新增
                    await this.handleAutoDetectStatus(device, data);
                    break;
                default:
                // Ignore others like debug/adc
            }

        } catch (e) {
            this.logger.error(`Failed to handle MQTT message: ${e.message}`, e.stack);
        }
    }

    private async handleStatus(device: Device, data: any) {
        // 1. Update lastState
        // Filter out sensor data from state to keep it clean, or just save everything.
        // Assuming payload: { temp: 26.5, hum: 60, current: 1.2, power: true, mode: 'cool', source: 'ir_recv' }

        device.lastState = data;
        await this.deviceRepo.save(device);

        // 2. Save Sensor History
        if (data.temp !== undefined || data.hum !== undefined || data.current !== undefined) {
            const reading = this.sensorRepo.create({
                deviceId: device.id,
                temperature: data.temp,
                humidity: data.hum,
                current: data.current
            });
            await this.sensorRepo.save(reading);
        }

        // 3. Log Sync source
        if (data.source === 'ir_recv') {
            const log = this.auditRepo.create({
                deviceId: device.id,
                action: AuditAction.SYNC_IR,
                details: data,
            });
            await this.auditRepo.save(log);
        }

        // 4. Trigger Rule Engine
        // Non-blocking call to avoid delaying the status processing
        this.routinesService.evaluateRules(device.id, {
            temp: data.temp,
            hum: data.hum
        }).catch(err => this.logger.error(`Rule Engine Error: ${err.message}`));

        // ✨ 5. 触发WebSocket推送
        this.eventEmitter.emit('device.status', {
            deviceId: device.id,
            userId: device.userId,
            data,
        });
    }

    private async handleEvent(device: Device, data: any) {
        if (data.type === 'ghost') {
            const log = this.auditRepo.create({
                deviceId: device.id,
                action: AuditAction.EVENT_GHOST,
                details: data,
            });
            await this.auditRepo.save(log);
            this.logger.warn(`Ghost Operation detected on Device ${device.id}`);

            // ✨ 触发WebSocket推送
            this.eventEmitter.emit('device.ghost', {
                userId: device.userId,
                deviceId: device.id,
                deviceName: device.name,
            });
        }
    }

    private async handleLearn(device: Device, data: any) {
        // Payload: { key: "cool_26", raw: "..." }
        if (data.key && data.raw) {
            if (!device.irConfig) device.irConfig = {};
            device.irConfig[data.key] = data.raw;
            await this.deviceRepo.save(device);

            this.auditRepo.save(this.auditRepo.create({
                deviceId: device.id,
                action: AuditAction.LEARN,
                details: { key: data.key }
            }));

            // ✨ 标记学习成功
            this.learnService.markSuccess(device.uuid);

            this.logger.log(`Learned IR Code for ${data.key} on Device ${device.id}`);
        }
    }

    // ===== 自动协议检测 =====

    /**
     * 处理自动检测结果
     */
    private async handleAutoDetectResult(device: Device, data: any) {
        if (data.success && data.isAC) {
            // ✅ 识别成功，自动保存品牌配置
            device.brandConfig = JSON.stringify({
                brandId: data.protocol,
                model: data.model || 0,
                autoDetected: true,
                detectedAt: new Date(),
                description: data.description
            });

            await this.deviceRepo.save(device);

            this.logger.log(`Device ${device.id} auto-detected: ${data.protocol} model ${data.model}`);

            // WebSocket推送成功消息
            this.eventEmitter.emit('device.autodetect.success', {
                deviceId: device.id,
                userId: device.userId,
                brand: data.protocol,
                model: data.model,
                state: {
                    power: data.power,
                    mode: data.mode,
                    temp: data.temp,
                    fan: data.fan
                }
            });
        } else {
            // ❌ 识别失败
            this.logger.warn(`Device ${device.id} auto-detect failed (${data.protocol})`);

            // WebSocket推送失败消息
            this.eventEmitter.emit('device.autodetect.failed', {
                deviceId: device.id,
                userId: device.userId,
                protocol: data.protocol,
                suggestion: 'please_manual_select'
            });
        }
    }

    /**
      * 处理自动检测状态更新
      */
    private async handleAutoDetectStatus(device: Device, data: any) {
        // WebSocket推送状态更新
        this.eventEmitter.emit('device.autodetect.status', {
            deviceId: device.id,
            userId: device.userId,
            status: data.status,
            message: data.message,
            timeout: data.timeout
        });

        this.logger.debug(`Device ${device.id} auto-detect status: ${data.status}`);
    }
}
