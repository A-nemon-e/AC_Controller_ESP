import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  CreateDateColumn,
  UpdateDateColumn,
  Index,
} from 'typeorm';

export interface WeatherData {
  temperature: number;
  humidity: number;
  condition: string;
  windSpeed: number;
  windDirection: string;
  pressure: number;
  visibility: number;
  uvIndex: number;
  aqi: number;
  sunrise: string;
  sunset: string;
  forecast: ForecastItem[];
}

export interface ForecastItem {
  date: string;
  tempHigh: number;
  tempLow: number;
  condition: string;
}

@Entity('weather_cache')
export class WeatherCache {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column()
  @Index()
  deviceId: string;

  @Column()
  @Index()
  city: string;

  @Column({ type: 'simple-json' })
  data: WeatherData;

  @Column({ default: 30 })
  ttlMinutes: number;

  @Column({ nullable: true })
  expireTime: Date;

  @CreateDateColumn()
  createdAt: Date;

  @UpdateDateColumn()
  updatedAt: Date;

  isExpired(): boolean {
    const expiryTime = new Date(
      this.updatedAt.getTime() + this.ttlMinutes * 60 * 1000,
    );
    return new Date() > expiryTime;
  }
}