import { Injectable, Logger } from '@nestjs/common';
import { MqttService } from '../mqtt/mqtt.service';

export interface LearningStatus {
    key: string;
    status: 'waiting' | 'success' | 'timeout';
}

@Injectable()
export class LearnService {
    private readonly logger = new Logger(LearnService.name);
    private learningStatus = new Map<string, LearningStatus>();

    constructor(private mqttService: MqttService) { }

    async startLearning(userId: number, deviceUuid: string, key: string) {
        const topic = `ac/user_${userId}/dev_${deviceUuid}/learn/start`;
        const payload = JSON.stringify({ key });

        this.mqttService.publish(topic, payload);
        this.logger.log(`Started learning for ${deviceUuid}, key: ${key}`);

        // 记录学习状态
        this.learningStatus.set(deviceUuid, { key, status: 'waiting' });

        // 30秒后标记为超时
        setTimeout(() => {
            const status = this.learningStatus.get(deviceUuid);
            if (status && status.status === 'waiting') {
                status.status = 'timeout';
            }
        }, 30000);

        return { message: 'Learning started', key };
    }

    getStatus(deviceUuid: string): LearningStatus | { status: 'idle' } {
        return this.learningStatus.get(deviceUuid) || { status: 'idle' };
    }

    markSuccess(deviceUuid: string) {
        const status = this.learningStatus.get(deviceUuid);
        if (status) {
            status.status = 'success';
        }
    }

    clearStatus(deviceUuid: string) {
        this.learningStatus.delete(deviceUuid);
    }
}
