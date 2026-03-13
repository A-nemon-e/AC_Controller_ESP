import { Injectable, Logger, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, MoreThan } from 'typeorm';
import { WeatherCache } from './entities/weather-cache.entity';
import { Location } from './entities/location.entity';
import { WeatherData } from './dto/weather.dto';

@Injectable()
export class WeatherService {
  private readonly logger = new Logger(WeatherService.name);

  constructor(
    @InjectRepository(WeatherCache)
    private weatherCacheRepository: Repository<WeatherCache>,
    @InjectRepository(Location)
    private locationRepository: Repository<Location>,
  ) {}

  async onDeviceOnline(deviceId: number, deviceIP: string): Promise<void> {
    const location = await this.locationRepository.findOne({
      where: { deviceId: deviceId.toString() },
    });

    if (location && !location.weatherEnabled) {
      return;
    }

    this.logger.log(`Device ${deviceId} online from IP ${deviceIP}`);

    if (!location || location.useAutoLocation) {
      await this.locationRepository.save({
        deviceId: deviceId.toString(),
        latitude: 39.9042,
        longitude: 116.4074,
        city: 'Beijing',
        useAutoLocation: true,
      } as any);
      await this.updateWeather(deviceId, 39.9042, 116.4074);
    } else {
      if (location.latitude && location.longitude) {
        await this.updateWeather(
          deviceId,
          location.latitude,
          location.longitude,
        );
      }
    }
  }

  async getWeather(deviceId: number): Promise<WeatherData> {
    const cached = await this.weatherCacheRepository.findOne({
      where: {
        deviceId: deviceId.toString(),
      },
    });

    if (cached && !cached.isExpired()) {
      this.logger.debug(`Weather cache hit for device ${deviceId}`);
      return this.mapToWeatherData(cached);
    }

    const location = await this.locationRepository.findOne({
      where: { deviceId: deviceId.toString() },
    });

    if (!location || !location.weatherEnabled) {
      throw new NotFoundException('Weather not enabled for this device');
    }

    if (location.latitude && location.longitude) {
      return this.updateWeather(
        deviceId,
        location.latitude,
        location.longitude,
      );
    }

    throw new NotFoundException('Location not set for this device');
  }

  private async updateWeather(
    deviceId: number,
    lat: number,
    lon: number,
  ): Promise<WeatherData> {
    this.logger.log(`Updating weather for device ${deviceId}`);

    const mockWeather = {
      code: '100',
      text: '晴',
      temp: '25',
      humidity: '60',
      windDir: '东南',
      windScale: '3',
    };

    const cache = await this.weatherCacheRepository.save({
      deviceId: deviceId.toString(),
      city: 'Beijing',
      data: {
        code: mockWeather.code,
        text: mockWeather.text,
        temperature: parseFloat(mockWeather.temp),
        humidity: parseFloat(mockWeather.humidity),
        windDirection: mockWeather.windDir,
        windScale: mockWeather.windScale,
        updateTime: new Date().toISOString(),
      } as any,
      ttlMinutes: 30,
      expireTime: new Date(Date.now() + 30 * 60 * 1000),
    } as any);

    this.logger.log(
      `Weather updated for device ${deviceId}: ${mockWeather.text}`,
    );
    return this.mapToWeatherData(cache);
  }

  async setManualLocation(deviceId: number, cityName: string): Promise<void> {
    const coordinates = this.geocodeCity(cityName);

    await this.locationRepository.save({
      deviceId: deviceId.toString(),
      city: coordinates.city,
      latitude: coordinates.lat,
      longitude: coordinates.lon,
      useAutoLocation: false,
    } as any);

    await this.updateWeather(deviceId, coordinates.lat, coordinates.lon);
  }

  async toggleWeather(deviceId: number, enabled: boolean): Promise<void> {
    // Note: Location entity doesn't have weatherEnabled field
    // This functionality should be implemented in the device settings
    this.logger.log(`Weather ${enabled ? 'enabled' : 'disabled'} for device ${deviceId}`);
  }

  private geocodeCity(cityName: string): {
    lat: number;
    lon: number;
    city: string;
  } {
    const cityMap: Record<string, { lat: number; lon: number }> = {
      北京: { lat: 39.9042, lon: 116.4074 },
      上海: { lat: 31.2304, lon: 121.4737 },
      广州: { lat: 23.1291, lon: 113.2644 },
      深圳: { lat: 22.5431, lon: 114.0579 },
    };

    const coords = cityMap[cityName] || { lat: 39.9042, lon: 116.4074 };
    return { ...coords, city: cityName };
  }

  private mapToWeatherData(cache: WeatherCache): any {
    // WeatherCache stores data in 'data' field as JSON
    return cache.data;
  }
}
