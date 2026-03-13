import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { AdminService } from './admin.service';
import { AdminStatsController } from './admin-stats.controller';
import { AdminUsersController } from './admin-users.controller';
import { AdminDevicesController } from './admin-devices.controller';
import { ActivityLog } from './activity-log.entity';
import { User } from '../users/user.entity';
import { Device } from '../devices/device.entity';
import { DeviceBinding } from '../devices/device-binding.entity';
import { DeviceBindingService } from '../devices/device-binding.service';
import { MqttModule } from '../mqtt/mqtt.module';
import { UsersModule } from '../users/users.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([ActivityLog, User, Device, DeviceBinding]),
    MqttModule,
    UsersModule,
  ],
  controllers: [
    AdminStatsController,
    AdminUsersController,
    AdminDevicesController,
  ],
  providers: [AdminService, DeviceBindingService],
  exports: [AdminService],
})
export class AdminModule {}
