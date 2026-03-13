import {
  Controller,
  Get,
  Post,
  Patch,
  Param,
  Body,
  Request,
  UseGuards,
  ParseIntPipe,
} from '@nestjs/common';
import type { Request as ExpressRequest } from 'express';
import { WeatherService } from './weather.service';
import { WeatherData, SetLocationDto } from './dto/weather.dto';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';

@Controller('weather')
@UseGuards(JwtAuthGuard)
export class WeatherController {
  constructor(private weatherService: WeatherService) {}

  @Get(':deviceId')
  async getWeather(
    @Request() _req: ExpressRequest,
    @Param('deviceId', ParseIntPipe) deviceId: number,
  ): Promise<WeatherData> {
    return this.weatherService.getWeather(deviceId);
  }

  @Post(':deviceId/location')
  async setLocation(
    @Request() _req: ExpressRequest,
    @Param('deviceId', ParseIntPipe) deviceId: number,
    @Body() dto: SetLocationDto,
  ): Promise<void> {
    await this.weatherService.setManualLocation(deviceId, dto.cityName);
  }

  @Patch(':deviceId/toggle')
  async toggleWeather(
    @Request() _req: ExpressRequest,
    @Param('deviceId', ParseIntPipe) deviceId: number,
    @Body('enabled') enabled: boolean,
  ): Promise<void> {
    await this.weatherService.toggleWeather(deviceId, enabled);
  }
}
