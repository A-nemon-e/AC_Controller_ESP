import { Test, TestingModule } from '@nestjs/testing';
import { WeatherService } from './services/weather.service';
import { WeatherController } from './controllers/weather.controller';
import { WeatherCache } from './entities/weather-cache.entity';
import { getRepositoryToken } from '@nestjs/typeorm';
import { HttpService } from '@nestjs/axios';
import { ConfigService } from '@nestjs/config';
import { of } from 'rxjs';

const mockRepository = {
  findOne: jest.fn(),
  save: jest.fn(),
};

const mockHttpService = {
  get: jest.fn(),
};

const mockConfigService = {
  get: jest.fn().mockReturnValue('test-key'),
};

describe('WeatherModule', () => {
  let controller: WeatherController;
  let service: WeatherService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [WeatherController],
      providers: [
        WeatherService,
        {
          provide: getRepositoryToken(WeatherCache),
          useValue: mockRepository,
        },
        {
          provide: HttpService,
          useValue: mockHttpService,
        },
        {
          provide: ConfigService,
          useValue: mockConfigService,
        },
      ],
    }).compile();

    controller = module.get<WeatherController>(WeatherController);
    service = module.get<WeatherService>(WeatherService);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
    expect(service).toBeDefined();
  });

  describe('getWeather', () => {
    it('should return weather data from API', async () => {
      const mockWeatherResponse = {
        code: '200',
        location: [{ id: '101010100' }],
      };

      const mockNowResponse = {
        code: '200',
        now: {
          temp: '25',
          humidity: '60',
          text: '晴',
          windScale: '3',
          windDir: '东南',
          pressure: '1010',
          vis: '10',
        },
      };

      const mockForecastResponse = {
        code: '200',
        daily: [
          {
            fxDate: '2024-01-01',
            tempMax: '26',
            tempMin: '15',
            textDay: '晴',
          },
        ],
      };

      const mockAirResponse = {
        code: '200',
        now: { aqi: '50' },
      };

      mockHttpService.get
        .mockReturnValueOnce(of({ data: mockWeatherResponse }))
        .mockReturnValueOnce(of({ data: mockNowResponse }))
        .mockReturnValueOnce(of({ data: mockForecastResponse }))
        .mockReturnValueOnce(of({ data: mockAirResponse }));

      mockRepository.findOne.mockResolvedValue(null);
      mockRepository.save.mockResolvedValue({});

      const result = await service.getWeather('北京');

      expect(result).toBeDefined();
      expect(result.temperature).toBe(25);
      expect(result.condition).toBe('晴');
    });

    it('should return cached weather if not expired', async () => {
      const cachedData = {
        data: {
          temperature: 20,
          humidity: 50,
          condition: '多云',
          windSpeed: 2,
          windDirection: '北',
          pressure: 1015,
          visibility: 8,
          uvIndex: 3,
          aqi: 80,
          sunrise: '06:00',
          sunset: '18:00',
          forecast: [],
        },
        isExpired: jest.fn().mockReturnValue(false),
      };

      mockRepository.findOne.mockResolvedValue(cachedData);

      const result = await service.getWeather('上海');

      expect(result).toEqual(cachedData.data);
      expect(mockHttpService.get).not.toHaveBeenCalled();
    });
  });
});