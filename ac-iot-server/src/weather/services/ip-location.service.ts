import { Injectable, Logger } from '@nestjs/common';
import { HttpService } from '@nestjs/axios';
import { ConfigService } from '@nestjs/config';
import { firstValueFrom } from 'rxjs';

interface IPLocationResponse {
  status: string;
  info: string;
  province: string;
  city: string;
  adcode: string;
  rectangle: string;
}

interface IPApiResponse {
  status: string;
  country: string;
  countryCode: string;
  region: string;
  regionName: string;
  city: string;
  lat: number;
  lon: number;
  isp: string;
}

@Injectable()
export class IpLocationService {
  private readonly logger = new Logger(IpLocationService.name);

  constructor(
    private httpService: HttpService,
    private configService: ConfigService,
  ) {}

  async getLocationByIP(ip: string): Promise<{
    city: string;
    province?: string;
    adcode?: string;
    latitude?: number;
    longitude?: number;
  } | null> {
    // 排除私有IP
    if (this.isPrivateIP(ip)) {
      this.logger.warn(`Private IP detected: ${ip}, using default location`);
      return null;
    }

    // 优先使用高德IP定位（国内）
    const amapKey = this.configService.get<string>('AMAP_KEY');
    if (amapKey) {
      try {
        const amapResult = await this.getLocationFromAmap(ip, amapKey);
        if (amapResult) return amapResult;
      } catch (error) {
        this.logger.warn(`Amap IP lookup failed: ${error.message}`);
      }
    }

    // 备用：使用 IP-API（国际）
    try {
      return await this.getLocationFromIPApi(ip);
    } catch (error) {
      this.logger.warn(`IP-API lookup failed: ${error.message}`);
    }

    return null;
  }

  private async getLocationFromAmap(
    ip: string,
    key: string,
  ): Promise<{ city: string; province: string; adcode: string } | null> {
    const url = `https://restapi.amap.com/v3/ip?ip=${ip}&key=${key}`;
    
    const response = await firstValueFrom(
      this.httpService.get<IPLocationResponse>(url),
    );

    if (response.data.status === '1' && response.data.city) {
      return {
        city: response.data.city.replace('市', ''),
        province: response.data.province,
        adcode: response.data.adcode,
      };
    }

    return null;
  }

  private async getLocationFromIPApi(
    ip: string,
  ): Promise<{ city: string; latitude: number; longitude: number } | null> {
    const url = `http://ip-api.com/json/${ip}?fields=status,country,regionName,city,lat,lon&lang=zh-CN`;
    
    const response = await firstValueFrom(
      this.httpService.get<IPApiResponse>(url),
    );

    if (response.data.status === 'success') {
      return {
        city: response.data.city,
        latitude: response.data.lat,
        longitude: response.data.lon,
      };
    }

    return null;
  }

  private isPrivateIP(ip: string): boolean {
    const privateRanges = [
      /^10\./,
      /^172\.(1[6-9]|2[0-9]|3[01])\./,
      /^192\.168\./,
      /^127\./,
      /^0\./,
      /^::1$/,
      /^fc00:/i,
      /^fe80:/i,
    ];
    
    return privateRanges.some((range) => range.test(ip));
  }
}