import {
  Injectable,
  Logger,
  NotFoundException,
  ForbiddenException,
  ConflictException,
  BadRequestException,
} from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Device } from './device.entity';
import { DeviceBinding } from './device-binding.entity';
import { CreateDeviceDto } from './dto/create-device.dto';
import { MqttService } from '../mqtt/mqtt.service';

@Injectable()
export class DeviceBindingService {
  private readonly logger = new Logger(DeviceBindingService.name);

  constructor(
    @InjectRepository(Device)
    private deviceRepository: Repository<Device>,
    @InjectRepository(DeviceBinding)
    private bindingRepository: Repository<DeviceBinding>,
    private mqttService: MqttService,
  ) {}

  async bindDevice(
    userId: number,
    createDeviceDto: CreateDeviceDto,
  ): Promise<Device> {
    const { uuid, mac } = createDeviceDto;

    let device = await this.deviceRepository.findOne({
      where: { uuid },
      relations: ['user'],
    });

    if (device) {
      if (device.userId === userId && device.bindStatus === 'bound') {
        this.logger.warn(`Device ${uuid} already bound to current user`);
        throw new ConflictException('Device already bound to your account');
      }

      if (device.userId && device.userId !== userId) {
        if (!device.isRebindable) {
          this.logger.warn(`Device ${uuid} bound to another user`);
          throw new ForbiddenException('Device bound to another user');
        }

        await this.recordBinding(
          device.id,
          device.userId,
          'unbind',
          'reclaimed_by_another_user',
        );
        this.logger.log(
          `Device ${uuid} reclaimed from user ${device.userId} to ${userId}`,
        );
      }

      if (device.backupConfig) {
        this.logger.log(`Restoring config for device ${uuid}`);
        device.brandConfig =
          device.backupConfig.brandConfig || device.brandConfig;
        device.irConfig = device.backupConfig.irConfig || device.irConfig;
      }

      device.userId = userId;
      device.bindStatus = 'bound';
      device.lastBindTime = new Date();
      device.boundUserId = userId;
    } else {
      device = this.deviceRepository.create({
        ...createDeviceDto,
        userId,
        bindStatus: 'bound',
        lastBindTime: new Date(),
        boundUserId: userId,
      });
    }

    const saved = await this.deviceRepository.save(device);

    await this.recordBinding(saved.id, userId, 'bind', 'initial_binding');

    await this.pushBindingConfig(saved);

    return saved;
  }

  async unbindDevice(
    userId: number,
    deviceId: number,
    options: { backupConfig?: boolean; reason?: string } = {},
  ): Promise<void> {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    if (device.userId !== userId) {
      throw new ForbiddenException('Not your device');
    }

    if (options.backupConfig !== false) {
      device.backupConfig = {
        brandConfig: device.brandConfig,
        irConfig: device.irConfig,
        micConfig: device.micConfig,
        setupStatus: device.setupStatus,
        backedUpAt: new Date().toISOString(),
      };
      device.isRebindable = true;
      this.logger.log(`Config backed up for device ${device.uuid}`);
    }

    (device as any).userId = null;
    device.bindStatus = 'unbound';
    device.lastUnbindTime = new Date();
    (device as any).boundUserId = null;

    await this.deviceRepository.save(device);

    await this.recordBinding(
      deviceId,
      userId,
      'unbind',
      options.reason || 'user_initiated',
    );

    await this.pushUnbindConfig(device);

    this.logger.log(`Device ${device.uuid} unbound by user ${userId}`);
  }

  async rebindDevice(userId: number, deviceId: number): Promise<Device> {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    if (!device.isRebindable || !device.backupConfig) {
      throw new BadRequestException('Device not eligible for rebind');
    }

    device.userId = userId;
    device.bindStatus = 'bound';
    device.lastBindTime = new Date();
    device.boundUserId = userId;

    const saved = await this.deviceRepository.save(device);

    await this.recordBinding(deviceId, userId, 'rebind', 'config_restored');

    this.logger.log(`Device ${device.uuid} rebound by user ${userId}`);

    return saved;
  }

  async getBindingHistory(deviceId: number): Promise<DeviceBinding[]> {
    return this.bindingRepository.find({
      where: { deviceId },
      order: { timestamp: 'DESC' },
      take: 50,
    });
  }

  private async recordBinding(
    deviceId: number,
    userId: number,
    action: 'bind' | 'unbind' | 'rebind',
    reason: string,
  ): Promise<void> {
    await this.bindingRepository.save({
      deviceId,
      userId,
      action,
      reason,
      timestamp: new Date(),
    });
  }

  private async pushBindingConfig(device: Device): Promise<void> {
    const macClean = device.uuid.replace(/:/g, '');
    const topic = `ac/config/${macClean}`;
    const payload = {
      userId: device.userId,
      deviceId: device.id,
      bindStatus: 'bound',
      timestamp: Date.now(),
    };

    await this.mqttService.publish(topic, JSON.stringify(payload));
  }

  private async pushUnbindConfig(device: Device): Promise<void> {
    const topic = `ac/user_${device.boundUserId}/dev_${device.uuid}/config/update`;
    const payload = {
      userId: 0,
      deviceId: 0,
      bindStatus: 'unbound',
      action: 'reset',
      timestamp: Date.now(),
    };

    await this.mqttService.publish(topic, JSON.stringify(payload));
  }
}
