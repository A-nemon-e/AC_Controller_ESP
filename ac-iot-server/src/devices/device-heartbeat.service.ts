import { Injectable, Logger } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Cron, CronExpression } from '@nestjs/schedule';
import { Device } from './device.entity';
import { EventEmitter2 } from '@nestjs/event-emitter';

@Injectable()
export class DeviceHeartbeatService {
  private readonly logger = new Logger(DeviceHeartbeatService.name);
  private deviceLastSeen: Map<number, number> = new Map();

  constructor(
    @InjectRepository(Device)
    private deviceRepository: Repository<Device>,
    private eventEmitter: EventEmitter2,
  ) {}

  onDeviceHeartbeat(deviceId: number): void {
    this.deviceLastSeen.set(deviceId, Date.now());
  }

  @Cron(CronExpression.EVERY_5_MINUTES)
  async checkOfflineDevices(): Promise<void> {
    const now = Date.now();
    const offlineThreshold = 5 * 60 * 1000;

    for (const [deviceId, lastSeen] of this.deviceLastSeen.entries()) {
      if (now - lastSeen > offlineThreshold) {
        await this.markDeviceOffline(deviceId);
      }
    }
  }

  private async markDeviceOffline(deviceId: number): Promise<void> {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (device && device.isOnline) {
      device.isOnline = false;
      await this.deviceRepository.save(device);

      this.eventEmitter.emit('device.offline', { deviceId });
      this.logger.log(`Device ${device.uuid} marked as offline`);
    }
  }
}
