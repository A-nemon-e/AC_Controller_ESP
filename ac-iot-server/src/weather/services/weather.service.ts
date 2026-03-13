import { Injectable, Logger, HttpException, HttpStatus } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { HttpService } from '@nestjs/axios';
import { ConfigService } from '@nestjs/config';
import { firstValueFrom } from 'rxjs';
import { WeatherCache, WeatherData } from '../entities/weather-cache.entity';

interface QWeatherNowResponse {
  code: string;
  now: {
    temp: string;
    humidity: string;
    text: string;
    windScale: string;
    windDir: string;
    pressure: string;
    vis: string;
  };
}

interface QWeatherForecastResponse {
  code: string;
  daily: Array<{
    fxDate: string;
    tempMax: string;
    tempMin: string;
    textDay: string;
  }>;
}

interface QWeatherAirResponse {
  code: string;
  now: {
    aqi: string;
  };
}

@Injectable()
export class WeatherService {
  private readonly logger = new Logger(WeatherService.name);

  constructor(
    @InjectRepository(WeatherCache)
    private weatherCacheRepo: Repository<WeatherCache>,
    private httpService: HttpService,
    private configService: ConfigService,
  ) {}

  async getWeather(city: string, forceRefresh = false): Promise<WeatherData> {
    // 检查缓存
    if (!forceRefresh) {
      const cached = await this.weatherCacheRepo.findOne({
        where: { city },
        order: { updatedAt: 'DESC' },
      });

      if (cached && !cached.isExpired()) {
        this.logger.debug(`Returning cached weather for ${city}`);
        return cached.data;
      }
    }

    // 获取新数据
    try {
      const weatherData = await this.fetchWeatherFromAPI(city);
      await this.cacheWeather(city, weatherData);
      return weatherData;
    } catch (error) {
      this.logger.error(`Failed to fetch weather for ${city}:`, error.message);
      
      // 尝试返回过期缓存
      const expired = await this.weatherCacheRepo.findOne({
        where: { city },
        order: { updatedAt: 'DESC' },
      });
      
      if (expired) {
        this.logger.warn(`Returning expired cache for ${city}`);
        return expired.data;
      }
      
      throw new HttpException(
        'Failed to fetch weather data',
        HttpStatus.SERVICE_UNAVAILABLE,
      );
    }
  }

  private async fetchWeatherFromAPI(city: string): Promise<WeatherData> {
    const apiKey = this.configService.get<string>('QWEATHER_KEY');
    const baseUrl = 'https://devapi.qweather.com/v7';

    const locationId = await this.getLocationId(city);

    // 并行获取实时天气、预报和空气质量
    const [nowRes, forecastRes, airRes] = await Promise.all([
      this.fetchWeatherData<QWeatherNowResponse>(
        `${baseUrl}/weather/now?location=${locationId}&key=${apiKey}`,
      ),
      this.fetchWeatherData<QWeatherForecastResponse>(
        `${baseUrl}/weather/3d?location=${locationId}&key=${apiKey}`,
      ),
      this.fetchWeatherData<QWeatherAirResponse>(
        `${baseUrl}/air/now?location=${locationId}&key=${apiKey}`,
      ),
    ]);

    return this.transformWeatherData(nowRes, forecastRes, airRes);
  }

  private async fetchWeatherData<T>(url: string): Promise<T> {
    const response = await firstValueFrom(this.httpService.get<T>(url));
    return response.data;
  }

  private async getLocationId(city: string): Promise<string> {
    const apiKey = this.configService.get<string>('QWEATHER_KEY');
    const url = `https://geoapi.qweather.com/v2/city/lookup?location=${encodeURIComponent(city)}&key=${apiKey}`;
    
    const response = await firstValueFrom(
      this.httpService.get<{ code: string; location: Array<{ id: string }> }>(url),
    );
    
    if (response.data.code !== '200' || !response.data.location?.length) {
      throw new Error(`City not found: ${city}`);
    }
    
    return response.data.location[0].id;
  }

  private transformWeatherData(
    now: QWeatherNowResponse,
    forecast: QWeatherForecastResponse,
    air: QWeatherAirResponse,
  ): WeatherData {
    return {
      temperature: parseInt(now.now.temp),
      humidity: parseInt(now.now.humidity),
      condition: now.now.text,
      windSpeed: parseInt(now.now.windScale),
      windDirection: now.now.windDir,
      pressure: parseInt(now.now.pressure),
      visibility: parseInt(now.now.vis),
      uvIndex: 0, // 需要额外API
      aqi: parseInt(air.now?.aqi || '0'),
      sunrise: '', // 需要日出日落API
      sunset: '',
      forecast: forecast.daily.slice(0, 3).map((day) => ({
        date: day.fxDate,
        tempHigh: parseInt(day.tempMax),
        tempLow: parseInt(day.tempMin),
        condition: day.textDay,
      })),
    };
  }

  private async cacheWeather(city: string, data: WeatherData): Promise<void> {
    const existing = await this.weatherCacheRepo.findOne({ where: { city } });
    
    if (existing) {
      existing.data = data;
      existing.updatedAt = new Date();
      await this.weatherCacheRepo.save(existing);
    } else {
      await this.weatherCacheRepo.save({
        city,
        data,
        ttlMinutes: 30,
      });
    }
  }

  async getWeatherConditionCode(condition: string, temperature?: number): Promise<string> {
    // 优先级1: 高温检查（30°C及以上触发火焰效果）
    if (temperature !== undefined && temperature >= 30) {
      return 'hot';
    }

    const conditionMap: Record<string, string> = {
      '晴': 'sunny',
      '多云': 'cloudy',
      '阴': 'overcast',
      '小雨': 'rain',
      '中雨': 'rain',
      '大雨': 'rain',
      '暴雨': 'rain',
      '阵雨': 'rain',
      '雷阵雨': 'rain',
      '雪': 'snow',
      '小雪': 'snow',
      '中雪': 'snow',
      '大雪': 'snow',
      '暴雪': 'snow',
      '雾': 'fog',
      '霾': 'fog',
      '风': 'wind',
      '大风': 'wind',
      '台风': 'wind',
      '飓风': 'wind',
    };
    return conditionMap[condition] || 'unknown';
  }
}