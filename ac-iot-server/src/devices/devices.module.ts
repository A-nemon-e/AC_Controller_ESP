import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { DevicesService } from './devices.service';
import { DevicesController } from './devices.controller';
import { Device } from './device.entity';
import { DeviceBinding } from './device-binding.entity';
import { MqttModule } from '../mqtt/mqtt.module';
import { AuditLog } from './audit-log.entity';
import { SensorReading } from './sensor-reading.entity';
import { DevicesGateway } from './devices.gateway';
import { LearnService } from './learn.service';
import { DeviceDiscoveryService } from './device-discovery.service';
import { DeviceBindingService } from './device-binding.service';
import { DeviceHeartbeatService } from './device-heartbeat.service';

@Module({
  imports: [
    TypeOrmModule.forFeature([Device, DeviceBinding, AuditLog, SensorReading]),
    MqttModule,
  ],
  controllers: [DevicesController],
  providers: [
    DevicesService,
    LearnService,
    DevicesGateway,
    DeviceDiscoveryService,
    DeviceBindingService,
    DeviceHeartbeatService,
  ],
  exports: [
    DevicesService,
    LearnService,
    DeviceDiscoveryService,
    DeviceBindingService,
    DeviceHeartbeatService,
  ],
})
export class DevicesModule {}
