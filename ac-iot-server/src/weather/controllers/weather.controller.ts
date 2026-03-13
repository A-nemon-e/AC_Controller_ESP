import {
  Controller,
  Get,
  Post,
  Body,
  Param,
  Query,
  UseGuards,
  HttpCode,
  HttpStatus,
} from '@nestjs/common';
import { JwtAuthGuard } from '../../auth/jwt-auth.guard';
import { WeatherService } from '../services/weather.service';
import { IpLocationService } from '../services/ip-location.service';
import { WeatherSchedulerService } from '../services/weather-scheduler.service';
// import { CurrentUser } from '../../auth/decorators/current-user.decorator';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Location } from '../entities/location.entity';

class UpdateLocationDto {
  deviceId: string;
  city?: string;
  useAutoLocation: boolean;
}

@Controller('weather')
@UseGuards(JwtAuthGuard)
export class WeatherController {
  constructor(
    private weatherService: WeatherService,
    private ipLocationService: IpLocationService,
    private weatherSchedulerService: WeatherSchedulerService,
    @InjectRepository(Location)
    private locationRepo: Repository<Location>,
  ) {}

  @Get(':city')
  async getWeather(@Param('city') city: string, @Query('refresh') refresh?: string) {
    const data = await this.weatherService.getWeather(city, refresh === 'true');
    const conditionCode = await this.weatherService.getWeatherConditionCode(
      data.condition,
    );
    
    return {
      success: true,
      data: {
        ...data,
        conditionCode,
      },
    };
  }

  @Get('ip-location/:ip')
  async getLocationByIP(@Param('ip') ip: string) {
    const location = await this.ipLocationService.getLocationByIP(ip);
    return {
      success: !!location,
      data: location,
    };
  }

  @Post('location')
  @HttpCode(HttpStatus.OK)
  async updateLocation(
    @Body() dto: UpdateLocationDto,
  ) {
    let location = await this.locationRepo.findOne({
      where: { deviceId: dto.deviceId },
    });

    if (location) {
      location.useAutoLocation = dto.useAutoLocation;
      if (dto.city) location.city = dto.city;
    } else {
      location = this.locationRepo.create({
        deviceId: dto.deviceId,
        city: dto.city,
        useAutoLocation: dto.useAutoLocation,
      });
    }

    await this.locationRepo.save(location);

    // 如果设置了城市，立即更新天气
    if (dto.city && !dto.useAutoLocation) {
      await this.weatherSchedulerService.updateDeviceWeather(
        dto.deviceId,
        dto.city,
      );
    }

    return {
      success: true,
      data: location,
    };
  }

  @Get('device/:deviceId')
  async getDeviceWeather(@Param('deviceId') deviceId: string) {
    const location = await this.locationRepo.findOne({
      where: { deviceId },
    });

    if (!location?.city) {
      return {
        success: false,
        error: 'Device location not configured',
      };
    }

    const data = await this.weatherService.getWeather(location.city);
    return {
      success: true,
      data,
    };
  }
}