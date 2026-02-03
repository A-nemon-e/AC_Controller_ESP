import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { UplinkService } from './uplink.service';
import { Device } from '../devices/device.entity';
import { SensorReading } from '../devices/sensor-reading.entity';
import { AuditLog } from '../devices/audit-log.entity';

import { RoutinesModule } from '../routines/routines.module';
import { DevicesModule } from '../devices/devices.module';

@Module({
    imports: [
        TypeOrmModule.forFeature([Device, SensorReading, AuditLog]),
        RoutinesModule,
        DevicesModule,
    ],
    providers: [UplinkService],
})
export class UplinkModule { }
