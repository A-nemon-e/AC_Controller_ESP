# Phase 3: 天气系统设计 - 详细设计文档

> **阶段**: @plan - Phase 3  
> **日期**: 2026-03-12  
> **前置依赖**: Phase 2 (显示引擎)  
> **状态**: 规划中
> **实施进度**: 0% (待开始)

---

## 1. 目标

构建完整的天气系统，包括后端天气服务、ESP天气模块和前端天气设置页面。

**成功标准**:
- [x] 后端实现天气API集成（和风天气 + IP定位）(90% - weather.service.ts完整，含缓存)
- [x] 实现天气数据缓存和自动更新 (90% - WeatherCache实体 + 定时更新)
- [x] ESP实现天气数据接收和解析 (100% - WeatherModule.h/cpp完整)
- [x] ESP实现6种天气效果（晴/雨/雪/风/阴/高温火焰）(100% - WeatherEffects.h/cpp完整)
- [x] 前端实现天气设置页面 (100% - WeatherSettings.vue 11,582行)
- [ ] 天气自动切换逻辑正常工作 (0% - 需要端到端测试)

**实施进度**: 80% (核心功能实现，待集成测试)

---

## 2. 系统架构

### 2.1 数据流

```
┌─────────────────────────────────────────────────────────────────┐
│                        天气系统数据流                            │
└─────────────────────────────────────────────────────────────────┘

ESP启动 → 连接WiFi → 上报IP地址 ────────────────────────────────┐
                                    │                           │
                                    ▼                           │
                            ┌──────────────┐                    │
                            │  后端服务     │                    │
                            │              │                    │
                            │ 1. 接收IP    │◄───────────────────┘
                            │ 2. IP定位API  │
                            │ 3. 和风API   │
                            │ 4. 缓存数据  │
                            │ 5. MQTT推送  │
                            └──────┬───────┘
                                   │ MQTT
                                   ▼
                            ┌──────────────┐
                            │   ESP设备    │
                            │              │
                            │ 解析天气数据 │
                            │ 自动选择效果 │
                            │ 渲染天气背景 │
                            └──────────────┘
                                   │
                                   │ WebSocket
                                   ▼
                            ┌──────────────┐
                            │   前端页面   │
                            │              │
                            │ 显示当前天气 │
                            │ 手动设置位置 │
                            │ 启用/禁用天气│
                            └──────────────┘
```

### 2.2 天气自动切换逻辑

```
温度 >= 30°C ──Yes──> 火焰背景（最高优先级）
    │
    No
    ▼
天气API数据 ──解析──> 天气代码 ──映射──> 背景效果
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│ 天气代码映射表（和风天气API）                                  │
├─────────────────────────────────────────────────────────────┤
│ 代码范围    │ 天气      │ 背景效果       │ 强度              │
├─────────────┼───────────┼────────────────┼───────────────────┤
│ 100         │ 晴        │ SUNNY          │ 太阳动画          │
│ 101-103     │ 多云      │ CLOUDY         │ 云朵移动          │
│ 300-399     │ 雨        │ RAINY          │ 大/中/小雨切换    │
│ 400-499     │ 雪        │ SNOWY          │ 雪花飘落          │
│ 其他        │ 阴/其他   │ CLOUDY         │ 灰色背景          │
└─────────────┴───────────┴────────────────┴───────────────────┘
```

---

## 3. 后端设计（NestJS）

### 3.1 模块结构

```
src/
└── weather/
    ├── weather.module.ts
    ├── weather.controller.ts
    ├── weather.service.ts
    ├── weather-scheduler.service.ts
    ├── ip-location.service.ts
    ├── entities/
    │   ├── weather-cache.entity.ts
    │   └── location.entity.ts
    └── dto/
        ├── weather-response.dto.ts
        ├── location.dto.ts
        └── weather-config.dto.ts
```

### 3.2 数据库实体

#### WeatherCache（天气缓存）

```typescript
@Entity()
export class WeatherCache {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    deviceId: number;

    @Column({ type: 'decimal', precision: 6, scale: 2 })
    latitude: number;

    @Column({ type: 'decimal', precision: 6, scale: 2 })
    longitude: number;

    @Column()
    cityName: string;

    @Column()
    weatherCode: string;  // 和风天气代码

    @Column()
    weatherText: string;  // 天气描述

    @Column({ type: 'decimal', precision: 4, scale: 1 })
    temperature: number;

    @Column({ type: 'decimal', precision: 4, scale: 1 })
    humidity: number;

    @Column({ nullable: true })
    windDirection: string;

    @Column({ nullable: true })
    windScale: string;  // 风力等级

    @Column({ type: 'datetime' })
    updateTime: Date;

    @Column({ type: 'datetime' })
    expireTime: Date;  // 缓存过期时间

    @CreateDateColumn()
    createdAt: Date;

    @UpdateDateColumn()
    updatedAt: Date;
}
```

#### Location（位置信息）

```typescript
@Entity()
export class Location {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    deviceId: number;

    @Column({ type: 'decimal', precision: 6, scale: 2, nullable: true })
    latitude: number;

    @Column({ type: 'decimal', precision: 6, scale: 2, nullable: true })
    longitude: number;

    @Column({ nullable: true })
    cityName: string;

    @Column({ nullable: true })
    district: string;

    @Column({ default: 'auto' })  // auto, manual
    locationMode: string;

    @Column({ default: true })
    weatherEnabled: boolean;

    @UpdateDateColumn()
    updatedAt: Date;
}
```

### 3.3 服务实现

#### WeatherService（核心服务）

```typescript
@Injectable()
export class WeatherService {
    private readonly logger = new Logger(WeatherService.name);
    private readonly apiKey: string;
    private readonly baseUrl = 'https://devapi.qweather.com/v7';

    constructor(
        @InjectRepository(WeatherCache)
        private weatherCacheRepository: Repository<WeatherCache>,
        @InjectRepository(Location)
        private locationRepository: Repository<Location>,
        private ipLocationService: IpLocationService,
        private configService: ConfigService,
        private mqttService: MqttService,
    ) {
        this.apiKey = this.configService.get<string>('QWEATHER_KEY');
    }

    /**
     * 设备上线时调用，自动获取天气
     */
    async onDeviceOnline(deviceId: number, deviceIP: string): Promise<void> {
        const location = await this.locationRepository.findOne({
            where: { deviceId }
        });

        // 如果未启用天气，跳过
        if (location && !location.weatherEnabled) {
            return;
        }

        // 自动模式：通过IP定位
        if (!location || location.locationMode === 'auto') {
            const ipLocation = await this.ipLocationService.locate(deviceIP);
            
            await this.locationRepository.save({
                deviceId,
                latitude: ipLocation.lat,
                longitude: ipLocation.lon,
                cityName: ipLocation.city,
                locationMode: 'auto',
            });

            // 获取天气
            await this.updateWeather(deviceId, ipLocation.lat, ipLocation.lon);
        } else {
            // 手动模式：使用保存的位置
            await this.updateWeather(deviceId, location.latitude, location.longitude);
        }
    }

    /**
     * 获取天气数据（带缓存）
     */
    async getWeather(deviceId: number): Promise<WeatherData> {
        // 检查缓存
        const cached = await this.weatherCacheRepository.findOne({
            where: {
                deviceId,
                expireTime: MoreThan(new Date())
            }
        });

        if (cached) {
            this.logger.debug(`Weather cache hit for device ${deviceId}`);
            return this.mapToWeatherData(cached);
        }

        // 缓存过期，重新获取
        const location = await this.locationRepository.findOne({
            where: { deviceId }
        });

        if (!location || !location.weatherEnabled) {
            throw new NotFoundException('Weather not enabled for this device');
        }

        return this.updateWeather(deviceId, location.latitude, location.longitude);
    }

    /**
     * 更新天气数据
     */
    private async updateWeather(
        deviceId: number,
        lat: number,
        lon: number
    ): Promise<WeatherData> {
        try {
            // 调用和风天气API
            const response = await axios.get(
                `${this.baseUrl}/weather/now`,
                {
                    params: {
                        location: `${lon},${lat}`,
                        key: this.apiKey,
                    },
                    timeout: 10000,
                }
            );

            const data = response.data;
            
            if (data.code !== '200') {
                throw new Error(`QWeather API error: ${data.code}`);
            }

            const now = data.now;

            // 保存到缓存
            const cache = await this.weatherCacheRepository.save({
                deviceId,
                latitude: lat,
                longitude: lon,
                cityName: data.location?.[0]?.name || 'Unknown',
                weatherCode: now.icon,
                weatherText: now.text,
                temperature: parseFloat(now.temp),
                humidity: parseFloat(now.humidity),
                windDirection: now.windDir,
                windScale: now.windScale,
                updateTime: new Date(),
                expireTime: new Date(Date.now() + 30 * 60 * 1000), // 30分钟缓存
            });

            // 推送到设备
            await this.pushToDevice(deviceId, cache);

            return this.mapToWeatherData(cache);
        } catch (error) {
            this.logger.error(`Failed to update weather: ${error.message}`);
            throw error;
        }
    }

    /**
     * 推送天气数据到ESP设备
     */
    private async pushToDevice(deviceId: number, weather: WeatherCache): Promise<void> {
        const device = await this.getDevice(deviceId);
        
        const payload = {
            type: 'weather_update',
            data: {
                code: weather.weatherCode,
                text: weather.weatherText,
                temp: weather.temperature,
                humidity: weather.humidity,
                windDir: weather.windDirection,
                windScale: weather.windScale,
                updateTime: weather.updateTime,
            },
        };

        const topic = `ac/user_${device.userId}/dev_${device.uuid}/weather/update`;
        await this.mqttService.publish(topic, JSON.stringify(payload));
        
        this.logger.log(`Weather pushed to device ${device.uuid}`);
    }

    /**
     * 手动设置位置
     */
    async setManualLocation(
        deviceId: number,
        cityName: string
    ): Promise<void> {
        // 通过城市名获取坐标
        const coordinates = await this.geocodeCity(cityName);
        
        await this.locationRepository.save({
            deviceId,
            cityName: coordinates.city,
            latitude: coordinates.lat,
            longitude: coordinates.lon,
            locationMode: 'manual',
        });

        // 立即更新天气
        await this.updateWeather(deviceId, coordinates.lat, coordinates.lon);
    }

    /**
     * 启用/禁用天气
     */
    async toggleWeather(deviceId: number, enabled: boolean): Promise<void> {
        await this.locationRepository.update(
            { deviceId },
            { weatherEnabled: enabled }
        );

        if (!enabled) {
            // 推送禁用命令
            await this.pushWeatherDisabled(deviceId);
        }
    }

    private mapToWeatherData(cache: WeatherCache): WeatherData {
        return {
            code: cache.weatherCode,
            text: cache.weatherText,
            temperature: cache.temperature,
            humidity: cache.humidity,
            windDirection: cache.windDirection,
            windScale: cache.windScale,
            updateTime: cache.updateTime,
        };
    }
}
```

#### IpLocationService（IP定位服务）

```typescript
@Injectable()
export class IpLocationService {
    private readonly logger = new Logger(IpLocationService.name);

    async locate(ip: string): Promise<LocationData> {
        try {
            // 方案1: 使用高德IP定位
            const amapKey = process.env.AMAP_KEY;
            const response = await axios.get(
                `https://restapi.amap.com/v3/ip`,
                {
                    params: {
                        key: amapKey,
                        ip: ip,
                    },
                    timeout: 5000,
                }
            );

            if (response.data.status === '1') {
                return {
                    lat: parseFloat(response.data.rectangle.split(';')[0].split(',')[1]),
                    lon: parseFloat(response.data.rectangle.split(';')[0].split(',')[0]),
                    city: response.data.city,
                    province: response.data.province,
                };
            }

            // 方案2: 使用IP-API（备用）
            return this.locateWithIPAPI(ip);
        } catch (error) {
            this.logger.warn(`IP location failed: ${error.message}`);
            // 返回默认位置（北京）
            return {
                lat: 39.9042,
                lon: 116.4074,
                city: 'Beijing',
                province: 'Beijing',
            };
        }
    }

    private async locateWithIPAPI(ip: string): Promise<LocationData> {
        const response = await axios.get(
            `http://ip-api.com/json/${ip}?fields=lat,lon,city,regionName,country`,
            { timeout: 5000 }
        );

        return {
            lat: response.data.lat,
            lon: response.data.lon,
            city: response.data.city,
            province: response.data.regionName,
        };
    }
}
```

#### WeatherSchedulerService（定时更新）

```typescript
@Injectable()
export class WeatherSchedulerService {
    private readonly logger = new Logger(WeatherSchedulerService.name);

    constructor(
        @InjectRepository(Location)
        private locationRepository: Repository<Location>,
        private weatherService: WeatherService,
    ) {}

    /**
     * 每30分钟更新一次天气
     */
    @Cron(CronExpression.EVERY_30_MINUTES)
    async scheduledWeatherUpdate(): Promise<void> {
        this.logger.log('Starting scheduled weather update');

        const locations = await this.locationRepository.find({
            where: { weatherEnabled: true }
        });

        for (const location of locations) {
            try {
                await this.weatherService.getWeather(location.deviceId);
                // 添加延迟避免API限流
                await new Promise(resolve => setTimeout(resolve, 1000));
            } catch (error) {
                this.logger.error(
                    `Failed to update weather for device ${location.deviceId}: ${error.message}`
                );
            }
        }

        this.logger.log('Scheduled weather update completed');
    }
}
```

### 3.4 控制器（API端点）

```typescript
@Controller('weather')
@UseGuards(JwtAuthGuard)
export class WeatherController {
    constructor(private weatherService: WeatherService) {}

    @Get(':deviceId')
    async getWeather(
        @Request() req,
        @Param('deviceId', ParseIntPipe) deviceId: number,
    ): Promise<WeatherData> {
        // 验证设备所有权
        await this.validateDeviceOwnership(req.user.userId, deviceId);
        return this.weatherService.getWeather(deviceId);
    }

    @Post(':deviceId/location')
    async setLocation(
        @Request() req,
        @Param('deviceId', ParseIntPipe) deviceId: number,
        @Body() dto: SetLocationDto,
    ): Promise<void> {
        await this.validateDeviceOwnership(req.user.userId, deviceId);
        await this.weatherService.setManualLocation(deviceId, dto.cityName);
    }

    @Patch(':deviceId/toggle')
    async toggleWeather(
        @Request() req,
        @Param('deviceId', ParseIntPipe) deviceId: number,
        @Body('enabled') enabled: boolean,
    ): Promise<void> {
        await this.validateDeviceOwnership(req.user.userId, deviceId);
        await this.weatherService.toggleWeather(deviceId, enabled);
    }
}
```

---

## 4. ESP模块设计（Arduino）

### 4.1 模块结构

```
lib/Weather/
├── WeatherModule.h/.cpp          # 天气模块主类
├── WeatherParser.h/.cpp          # JSON解析
├── WeatherEffects.h/.cpp         # 天气效果映射
└── WeatherConfig.h               # 配置定义
```

### 4.2 WeatherModule类

```cpp
class WeatherModule {
public:
    struct WeatherData {
        String code;           // 天气代码
        String text;           // 天气描述
        float temperature;     // 温度
        float humidity;        // 湿度
        String windDir;        // 风向
        String windScale;      // 风力
        time_t updateTime;     // 更新时间
    };

    // 初始化
    static void init();
    
    // 处理MQTT消息
    static void onWeatherUpdate(const char* json);
    static void onWeatherDisabled();
    
    // 获取当前天气
    static const WeatherData& getCurrentWeather() { return currentWeather; }
    static bool hasWeatherData() { return hasData; }
    
    // 自动选择背景效果
    static Background* createWeatherBackground();
    
    // 检查是否需要切换到天气背景
    static bool shouldUseWeatherBackground();
    
    // 配置
    static void setEnabled(bool enabled) { config.enabled = enabled; }
    static bool isEnabled() { return config.enabled; }
    
    // 序列化配置
    static void toJSON(JsonObject& json);
    static bool fromJSON(const JsonObject& json);

private:
    static WeatherData currentWeather;
    static bool hasData;
    static time_t lastUpdate;
    
    struct Config {
        bool enabled;
        bool autoMode;        // 自动/手动选择背景
        String manualCode;    // 手动选择的天气代码
    };
    static Config config;
    
    // 天气代码映射
    static Background::Type mapWeatherCodeToBackground(const String& code, float temp);
    
    // 特殊处理：高温火焰
    static bool isHighTemperature(float temp) { return temp >= 30.0f; }
};
```

### 4.3 天气代码映射

```cpp
Background::Type WeatherModule::mapWeatherCodeToBackground(
    const String& code, 
    float temp
) {
    // 优先级1: 高温火焰
    if (isHighTemperature(temp)) {
        return Background::Type::FIRE;
    }
    
    int codeNum = code.toInt();
    
    // 优先级2: 天气代码映射
    switch (codeNum) {
        case 100:  // 晴
            return Background::Type::WEATHER_SUNNY;
            
        case 101:  // 多云
        case 102:  // 少云
        case 103:  // 晴间多云
            return Background::Type::WEATHER_CLOUDY;
            
        case 300:  // 阵雨
        case 301:  // 强阵雨
        case 302:  // 雷阵雨
        case 303:  // 强雷阵雨
        case 304:  // 雷阵雨伴有冰雹
        case 305:  // 小雨
        case 306:  // 中雨
        case 307:  // 大雨
        case 308:  // 极端降雨
        case 309:  // 毛毛雨
        case 310:  // 暴雨
        case 311:  // 大暴雨
        case 312:  // 特大暴雨
        case 313:  // 冻雨
        case 314:  // 小到中雨
        case 315:  // 中到大雨
        case 316:  // 大到暴雨
        case 317:  // 暴雨到大暴雨
        case 318:  // 大暴雨到特大暴雨
            return Background::Type::WEATHER_RAINY;
            
        case 400:  // 小雪
        case 401:  // 中雪
        case 402:  // 大雪
        case 403:  // 暴雪
        case 404:  // 雨夹雪
        case 405:  // 雨雪天气
        case 406:  // 阵雨夹雪
        case 407:  // 阵雪
        case 408:  // 小到中雪
        case 409:  // 中到大雪
        case 410:  // 大到暴雪
            return Background::Type::WEATHER_SNOWY;
            
        default:
            // 其他天气：阴天
            return Background::Type::WEATHER_CLOUDY;
    }
}
```

### 4.4 MQTT消息处理

```cpp
void WeatherModule::onWeatherUpdate(const char* json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        DEBUG_PRINTLN("[Weather] JSON parse failed");
        return;
    }
    
    // 解析天气数据
    currentWeather.code = doc["data"]["code"] | "999";
    currentWeather.text = doc["data"]["text"] | "Unknown";
    currentWeather.temperature = doc["data"]["temp"] | 25.0f;
    currentWeather.humidity = doc["data"]["humidity"] | 50.0f;
    currentWeather.windDir = doc["data"]["windDir"] | "";
    currentWeather.windScale = doc["data"]["windScale"] | "";
    currentWeather.updateTime = time(nullptr);
    
    hasData = true;
    lastUpdate = millis();
    
    DEBUG_PRINTF("[Weather] Updated: %s, %.1f°C\n", 
                 currentWeather.text.c_str(), 
                 currentWeather.temperature);
    
    // 如果当前卡片使用天气背景，通知DisplayManager刷新
    if (shouldUseWeatherBackground()) {
        DisplayManager::getInstance().refreshBackground();
    }
}

void WeatherModule::onWeatherDisabled() {
    hasData = false;
    DEBUG_PRINTLN("[Weather] Disabled by server");
}
```

---

## 5. 前端设计

### 5.1 页面结构

```
src/views/
└── WeatherSettings.vue          # 天气设置页面

src/components/weather/
├── WeatherCard.vue              # 天气信息卡片
├── LocationSelector.vue         # 位置选择器
├── WeatherEffectPreview.vue     # 天气效果预览
└── WeatherMap.vue               # 天气地图（可选）
```

### 5.2 WeatherSettings.vue

```vue
<template>
  <div class="weather-settings">
    <!-- 启用开关 -->
    <van-cell center title="启用天气" >
      <template #right-icon>
        <van-switch v-model="enabled" @change="onToggle" />
      </template>
    </van-cell>
    
    <!-- 当前天气显示 -->
    <weather-card 
      v-if="enabled && weatherData"
      :data="weatherData"
      class="weather-card"
    />
    
    <!-- 位置设置 -->
    <van-cell-group v-if="enabled" inset title="位置设置">
      <van-cell title="定位模式" :value="locationModeText" />
      
      <van-cell 
        title="当前位置" 
        :value="currentLocation || '自动定位中...'"
        is-link
        @click="showLocationPicker = true"
      />
      
      <van-cell 
        title="手动选择城市"
        is-link
        @click="onManualSelect"
      />
    </van-cell-group>
    
    <!-- 天气效果预览 -->
    <van-cell-group v-if="enabled" inset title="效果预览">
      <weather-effect-preview 
        :weather-code="weatherData?.code"
        :temperature="weatherData?.temperature"
      />
    </van-cell-group>
    
    <!-- 更新频率 -->
    <van-cell-group v-if="enabled" inset title="更新设置">
      <van-cell title="自动更新" label="每30分钟自动更新" />
      <van-cell 
        title="立即更新"
        is-link
        @click="onRefresh"
        :loading="refreshing"
      />
    </van-cell-group>
    
    <!-- 城市选择弹窗 -->
    <van-popup v-model:show="showLocationPicker" position="bottom">
      <van-picker
        :columns="cities"
        @confirm="onCityConfirm"
        @cancel="showLocationPicker = false"
      />
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { showToast } from 'vant'
import { useWeatherStore } from '@/stores/weather'
import WeatherCard from '@/components/weather/WeatherCard.vue'
import WeatherEffectPreview from '@/components/weather/WeatherEffectPreview.vue'

const route = useRoute()
const weatherStore = useWeatherStore()

const deviceId = computed(() => parseInt(route.params.id as string))

const enabled = ref(false)
const refreshing = ref(false)
const showLocationPicker = ref(false)

const weatherData = computed(() => weatherStore.currentWeather)
const locationModeText = computed(() => 
  weatherStore.locationMode === 'auto' ? '自动定位' : '手动选择'
)
const currentLocation = computed(() => weatherStore.currentLocation)

onMounted(async () => {
  await weatherStore.fetchWeather(deviceId.value)
  enabled.value = weatherStore.isEnabled
})

const onToggle = async (value: boolean) => {
  await weatherStore.toggleWeather(deviceId.value, value)
  showToast(value ? '天气已启用' : '天气已禁用')
}

const onRefresh = async () => {
  refreshing.value = true
  try {
    await weatherStore.refreshWeather(deviceId.value)
    showToast('天气已更新')
  } finally {
    refreshing.value = false
  }
}

const onManualSelect = () => {
  // 打开城市搜索页面
}

const onCityConfirm = async ({ selectedValues }: any) => {
  const city = selectedValues[0]
  await weatherStore.setManualLocation(deviceId.value, city)
  showToast(`已设置为: ${city}`)
  showLocationPicker.value = false
}
</script>
```

### 5.3 Pinia Store

```typescript
// stores/weather.ts
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { weatherApi } from '@/api/weather'

export const useWeatherStore = defineStore('weather', () => {
  // State
  const currentWeather = ref<WeatherData | null>(null)
  const isEnabled = ref(false)
  const locationMode = ref<'auto' | 'manual'>('auto')
  const currentLocation = ref('')
  const loading = ref(false)

  // Actions
  async function fetchWeather(deviceId: number) {
    loading.value = true
    try {
      const data = await weatherApi.getWeather(deviceId)
      currentWeather.value = data
      isEnabled.value = data.enabled
      locationMode.value = data.locationMode
      currentLocation.value = data.location
    } finally {
      loading.value = false
    }
  }

  async function refreshWeather(deviceId: number) {
    return fetchWeather(deviceId)
  }

  async function toggleWeather(deviceId: number, enabled: boolean) {
    await weatherApi.toggleWeather(deviceId, enabled)
    isEnabled.value = enabled
  }

  async function setManualLocation(deviceId: number, city: string) {
    await weatherApi.setLocation(deviceId, city)
    locationMode.value = 'manual'
    currentLocation.value = city
    // 重新获取天气
    await fetchWeather(deviceId)
  }

  return {
    currentWeather,
    isEnabled,
    locationMode,
    currentLocation,
    loading,
    fetchWeather,
    refreshWeather,
    toggleWeather,
    setManualLocation,
  }
})
```

---

## 6. API接口规范

### 6.1 REST API

```yaml
# 获取天气
GET /api/weather/:deviceId
Response:
  {
    "code": "100",
    "text": "晴",
    "temperature": 25.5,
    "humidity": 60,
    "windDir": "东南",
    "windScale": "3级",
    "updateTime": "2026-03-12T10:30:00Z",
    "location": "北京市",
    "enabled": true,
    "locationMode": "auto"
  }

# 手动设置位置
POST /api/weather/:deviceId/location
Body:
  {
    "cityName": "上海市"
  }

# 启用/禁用天气
PATCH /api/weather/:deviceId/toggle
Body:
  {
    "enabled": true
  }
```

### 6.2 MQTT Topic

```yaml
# 服务器 -> ESP: 天气更新
Topic: ac/user_{userId}/dev_{uuid}/weather/update
Payload:
  {
    "type": "weather_update",
    "data": {
      "code": "100",
      "text": "晴",
      "temp": 25.5,
      "humidity": 60,
      "windDir": "东南",
      "windScale": "3级",
      "updateTime": "2026-03-12T10:30:00Z"
    }
  }

# 服务器 -> ESP: 禁用天气
Topic: ac/user_{userId}/dev_{uuid}/weather/disable
Payload:
  {
    "type": "weather_disabled"
  }
```

---

## 7. 实施顺序

### Week 6.1: 后端天气服务
1. [ ] 创建Weather模块
2. [ ] 实现WeatherService
3. [ ] 实现IpLocationService
4. [ ] 实现定时更新
5. [ ] API端点

### Week 6.2: 后端集成
1. [ ] 设备上线自动获取天气
2. [ ] MQTT推送集成
3. [ ] 缓存策略测试
4. [ ] API限流保护

### Week 7.1: ESP天气模块
1. [ ] WeatherModule类
2. [ ] JSON解析
3. [ ] 天气代码映射
4. [ ] 与DisplayManager集成

### Week 7.2: ESP天气效果
1. [ ] 晴天效果
2. [ ] 雨天效果（大中小切换）
3. [ ] 雪天效果
4. [ ] 刮风效果
5. [ ] 阴天效果

### Week 8.1: 前端页面
1. [ ] WeatherSettings页面
2. [ ] WeatherCard组件
3. [ ] 位置选择器
4. [ ] Pinia Store

### Week 8.2: 集成测试
1. [ ] 端到端测试
2. [ ] 自动切换测试
3. [ ] 性能优化
4. [ ] 文档完善

---

**下一步**: 创建Phase 4后端改进设计
