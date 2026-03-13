import {
  Module,
  NestModule,
  MiddlewareConsumer,
  RequestMethod,
} from '@nestjs/common';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ScheduleModule } from '@nestjs/schedule';
import { EventEmitterModule } from '@nestjs/event-emitter';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { MqttModule } from './mqtt/mqtt.module';
import { UsersModule } from './users/users.module';
import { User } from './users/user.entity';
import { UserSettings } from './users/user-settings.entity';
import { Device } from './devices/device.entity';
import { DeviceBinding } from './devices/device-binding.entity';
import { Routine } from './routines/routine.entity';
import { AuditLog } from './devices/audit-log.entity';
import { SensorReading } from './devices/sensor-reading.entity';
import { AuthModule } from './auth/auth.module';
import { DevicesModule } from './devices/devices.module';
import { RoutinesModule } from './routines/routines.module';
import { UplinkModule } from './uplink/uplink.module';
import { AgreementModule } from './agreement/agreement.module';
import { AdminModule } from './admin/admin.module';
import { WeatherModule } from './weather/weather.module';
import { UserAgreement } from './agreement/user-agreement.entity';
import { UserAgreementAcceptance } from './agreement/user-agreement-acceptance.entity';
import { PasswordHistory } from './auth/password-history.entity';
import { ActivityLog } from './admin/activity-log.entity';
import { WeatherCache } from './weather/entities/weather-cache.entity';
import { Location } from './weather/entities/location.entity';
import { LoggerMiddleware } from './common/middleware/logger.middleware';

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
      envFilePath:
        process.env.NODE_ENV === 'production'
          ? ['.env.production', '.env']
          : ['.env.development', '.env'],
    }),
    TypeOrmModule.forRootAsync({
      imports: [ConfigModule],
      inject: [ConfigService],
      useFactory: (configService: ConfigService) => {
        const dbFile = configService.get<string>('DB_FILE');
        console.log('DEBUG: DB_FILE is', dbFile);

        if (!dbFile) {
          console.error('CRITICAL: DB_FILE is undefined! Check .env file.');
          return {
            type: 'sqlite',
            database: './fallback.db',
            entities: [
              User,
              UserSettings,
              Device,
              DeviceBinding,
              Routine,
              AuditLog,
              SensorReading,
              UserAgreement,
              UserAgreementAcceptance,
              PasswordHistory,
              ActivityLog,
              WeatherCache,
              Location,
            ],
            synchronize: true,
          };
        }

        console.log('DEBUG: Current Directory is', process.cwd());
        return {
          type: 'sqlite',
          database: dbFile,
          entities: [
            User,
            UserSettings,
            Device,
            DeviceBinding,
            Routine,
            AuditLog,
            SensorReading,
            UserAgreement,
            UserAgreementAcceptance,
            PasswordHistory,
            ActivityLog,
            WeatherCache,
            Location,
          ],
          synchronize: true,
        };
      },
    }),
    MqttModule,
    UsersModule,
    AuthModule,
    DevicesModule,
    RoutinesModule,
    UplinkModule,
    AgreementModule,
    AdminModule,
    WeatherModule,
    ScheduleModule.forRoot(),
    EventEmitterModule.forRoot(),
  ],
  controllers: [AppController],
  providers: [AppService],
})
export class AppModule implements NestModule {
  configure(consumer: MiddlewareConsumer) {
    consumer
      .apply(LoggerMiddleware)
      .forRoutes({ path: '*', method: RequestMethod.ALL });
  }
}
