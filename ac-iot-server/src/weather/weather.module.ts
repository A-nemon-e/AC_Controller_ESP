import { Module } from '@nestjs/common';
import { HttpModule } from '@nestjs/axios';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ScheduleModule } from '@nestjs/schedule';
import { WeatherService } from './services/weather.service';
import { WeatherController } from './controllers/weather.controller';
import { WeatherSchedulerService } from './services/weather-scheduler.service';
import { IpLocationService } from './services/ip-location.service';
import { MqttModule } from '../mqtt/mqtt.module';
import { WeatherCache } from './entities/weather-cache.entity';
import { Location } from './entities/location.entity';
import { Device } from '../devices/device.entity';

@Module({
  imports: [
    TypeOrmModule.forFeature([WeatherCache, Location, Device]),
    ScheduleModule.forRoot(),
    MqttModule,
    HttpModule,
  ],
  controllers: [WeatherController],
  providers: [
    WeatherService,
    WeatherSchedulerService,
    IpLocationService,
  ],
  exports: [WeatherService],
})
export class WeatherModule {}