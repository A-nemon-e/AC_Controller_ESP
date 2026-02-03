import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { DevicesService } from './devices.service';
import { DevicesController } from './devices.controller';
import { Device } from './device.entity';
import { MqttModule } from '../mqtt/mqtt.module';
import { AuditLog } from './audit-log.entity';
import { SensorReading } from './sensor-reading.entity';
import { DevicesGateway } from './devices.gateway';
import { LearnService } from './learn.service';

@Module({
    imports: [
        TypeOrmModule.forFeature([Device, AuditLog, SensorReading]),
        MqttModule,
    ],
    controllers: [DevicesController],
    providers: [DevicesService, LearnService, DevicesGateway],
    exports: [DevicesService, LearnService],
})
export class DevicesModule { }
