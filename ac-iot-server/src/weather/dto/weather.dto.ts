export class WeatherData {
  code: string;
  text: string;
  temperature: number;
  humidity: number;
  windDirection?: string;
  windScale?: string;
  updateTime: Date;
}

export class SetLocationDto {
  cityName: string;
}

export class WeatherConfigDto {
  enabled: boolean;
  locationMode: 'auto' | 'manual';
  location?: string;
}
