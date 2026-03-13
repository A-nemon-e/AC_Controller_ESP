import { Injectable, Logger } from '@nestjs/common';
import { Cron, CronExpression } from '@nestjs/schedule';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { WeatherService } from './weather.service';
import { MqttService } from '../../mqtt/mqtt.service';
import { Location } from '../entities/location.entity';
import { Device } from '../../devices/device.entity';

@Injectable()
export class WeatherSchedulerService {
  private readonly logger = new Logger(WeatherSchedulerService.name);
  // API限流保护：记录上次请求时间
  private lastRequestTime: Map<string, number> = new Map();
  private readonly minRequestInterval = 1000; // 1秒间隔

  constructor(
    @InjectRepository(Location)
    private locationRepo: Repository<Location>,
    @InjectRepository(Device)
    private deviceRepo: Repository<Device>,
    private weatherService: WeatherService,
    private mqttService: MqttService,
  ) {}

  @Cron(CronExpression.EVERY_30_MINUTES)
  async updateAllDeviceWeather() {
    this.logger.log('Starting scheduled weather update for all devices');
    
    try {
      const locations = await this.locationRepo.find({
        where: { weatherEnabled: true },
      });

      for (const location of locations) {
        if (location.city) {
          // 添加延迟避免API限流
          await this.delay(1000);
          await this.updateDeviceWeather(location.deviceId, location.city);
        }
      }

      this.logger.log(`Updated weather for ${locations.length} devices`);
    } catch (error) {
      this.logger.error('Failed to update weather:', error.message);
    }
  }

  async updateDeviceWeather(deviceId: string, city: string) {
    try {
      // API限流保护
      const now = Date.now();
      const lastRequest = this.lastRequestTime.get(deviceId) || 0;
      if (now - lastRequest < this.minRequestInterval) {
        await this.delay(this.minRequestInterval - (now - lastRequest));
      }
      this.lastRequestTime.set(deviceId, Date.now());

      const weather = await this.weatherService.getWeather(city);
      const conditionCode = await this.weatherService.getWeatherConditionCode(
        weather.condition,
        weather.temperature,
      );

      // 获取设备信息以构建正确的topic
      const device = await this.deviceRepo.findOne({
        where: { id: parseInt(deviceId) },
      });

      if (!device) {
        this.logger.warn(`Device ${deviceId} not found`);
        return;
      }

      if (!device.userId) {
        this.logger.warn(`Device ${deviceId} is not bound to any user`);
        return;
      }

      const payload = {
        type: 'weather_update',
        data: {
          conditionCode,
          temperature: weather.temperature,
          humidity: weather.humidity,
          windSpeed: weather.windSpeed,
          windDirection: weather.windDirection,
        },
      };

      // 使用设计文档规定的topic格式
      const topic = `ac/user_${device.userId}/dev_${device.uuid}/weather/update`;
      await this.mqttService.publish(topic, JSON.stringify(payload));
      this.logger.debug(`Weather sent to device ${deviceId} via topic ${topic}`);
    } catch (error) {
      this.logger.error(
        `Failed to update weather for device ${deviceId}:`,
        error.message,
      );
    }
  }

  private delay(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
}