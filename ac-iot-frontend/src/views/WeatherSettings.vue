<template>
  <div class="weather-settings-page">
    <!-- 当前天气 -->
    <van-cell-group inset title="当前天气" v-if="weatherStore.weatherData">
      <div class="weather-card">
        <div class="weather-main">
          <div class="weather-icon">{{ weatherIcon }}</div>
          <div class="weather-temp">
            <span class="temp-value">{{ weatherStore.currentTemperature }}</span>
            <span class="temp-unit">{{ weatherStore.temperatureUnit }}</span>
          </div>
        </div>
        <div class="weather-desc">{{ weatherStore.weatherData.current.condition.text }}</div>
        
        <div class="weather-details">
          <div class="detail-item">
            <van-icon name="graphic" />
            <span>湿度 {{ weatherStore.weatherData.current.humidity }}%</span>
          </div>
          <div class="detail-item">
            <van-icon name="like-o" />
            <span>风速 {{ weatherStore.weatherData.current.windSpeed }}m/s</span>
          </div>
          <div class="detail-item">
            <van-icon name="eye-o" />
            <span>能见度 {{ weatherStore.weatherData.current.visibility }}km</span>
          </div>
        </div>
        
        <div class="weather-location">
          <van-icon name="location-o" />
          <span>{{ weatherStore.formattedLocation }}</span>
        </div>
      </div>
    </van-cell-group>

    <!-- 位置设置 -->
    <van-cell-group inset title="位置设置" class="mt-12">
      <van-cell title="天气功能" center>
        <template #right-icon>
          <van-switch v-model="weatherStore.settings.enabled" @change="onToggleEnabled" />
        </template>
      </van-cell>
      
      <template v-if="weatherStore.settings.enabled">
        <van-cell title="位置类型" :value="locationTypeLabel" is-link @click="showLocationTypePicker = true" />
        
        <van-cell
          v-if="weatherStore.settings.locationType === 'manual'"
          title="当前位置"
          :value="weatherStore.settings.location?.name || '未设置'"
          is-link
          @click="showLocationSearch = true"
        />
        
        <van-cell
          v-else
          title="自动定位"
          value="使用中"
          is-link
          @click="detectLocation"
        >
          <template #right-icon>
            <van-button size="small" type="primary" :loading="weatherStore.loading">重新定位</van-button>
          </template>
        </van-cell>
      </template>
    </van-cell-group>

    <!-- 更新设置 -->
    <van-cell-group inset title="更新设置" class="mt-12" v-if="weatherStore.settings.enabled">
      <van-cell title="更新间隔" :value="`${weatherStore.settings.updateInterval}分钟`" is-link @click="showIntervalPicker = true" />
      
      <van-cell title="显示预报" center>
        <template #right-icon>
          <van-switch v-model="weatherStore.settings.showForecast" @change="saveSettings" />
        </template>
      </van-cell>
      
      <van-cell title="温度单位" center>
        <template #right-icon>
          <van-button size="small" type="primary" @click="toggleUnit">
            {{ weatherStore.settings.temperatureUnit === 'celsius' ? '°C' : '°F' }}
          </van-button>
        </template>
      </van-cell>
      
      <van-cell title="天气效果" center>
        <template #right-icon>
          <van-switch v-model="weatherStore.settings.displayEffects" @change="saveSettings" />
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 天气预报 -->
    <van-cell-group
      inset
      title="未来预报"
      class="mt-12"
      v-if="weatherStore.settings.enabled && weatherStore.settings.showForecast && weatherStore.weatherData?.forecast"
    >
      <div class="forecast-list">
        <div
          v-for="(day, index) in weatherStore.weatherData.forecast"
          :key="index"
          class="forecast-item"
        >
          <span class="forecast-date">{{ formatDate(day.date) }}</span>
          <span class="forecast-icon">{{ getWeatherIcon(day.condition.code) }}</span>
          <span class="forecast-temp">{{ day.minTemp }}° - {{ day.maxTemp }}°</span>
          <span class="forecast-condition">{{ day.condition.text }}</span>
        </div>
      </div>
    </van-cell-group>

    <!-- 最后更新时间 -->
    <div class="last-update" v-if="weatherStore.weatherData">
      最后更新: {{ formatLastUpdate }}
    </div>

    <!-- 选择器弹窗 -->
    <van-popup v-model:show="showLocationTypePicker" position="bottom" round>
      <van-picker
        title="位置类型"
        :columns="locationTypeColumns"
        @confirm="onLocationTypeConfirm"
        @cancel="showLocationTypePicker = false"
      />
    </van-popup>

    <van-popup v-model:show="showIntervalPicker" position="bottom" round>
      <van-picker
        title="更新间隔"
        :columns="intervalColumns"
        @confirm="onIntervalConfirm"
        @cancel="showIntervalPicker = false"
      />
    </van-popup>

    <van-popup v-model:show="showLocationSearch" position="bottom" round :style="{ height: '70%' }">
      <div class="location-search">
        <van-search
          v-model="searchQuery"
          placeholder="搜索城市"
          @search="searchLocation"
        />
        <van-list
          :finished="true"
          class="location-list"
        >
          <van-cell
            v-for="city in searchResults"
            :key="city.name"
            :title="city.name"
            :label="city.country"
            is-link
            @click="selectLocation(city)"
          />
        </van-list>
      </div>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useWeatherStore } from '@/stores/weather'
import { showToast } from 'vant'
import type { Location } from '@/types/weather'

const weatherStore = useWeatherStore()

// UI State
const showLocationTypePicker = ref(false)
const showIntervalPicker = ref(false)
const showLocationSearch = ref(false)
const searchQuery = ref('')
const searchResults = ref<Location[]>([
  { name: '北京', country: '中国' },
  { name: '上海', country: '中国' },
  { name: '广州', country: '中国' },
  { name: '深圳', country: '中国' },
  { name: '杭州', country: '中国' },
])

// Computed
const weatherIcon = computed(() => {
  const code = weatherStore.weatherData?.current.condition.code || 1000
  return getWeatherIcon(code)
})

const locationTypeLabel = computed(() => {
  return weatherStore.settings.locationType === 'auto' ? '自动定位' : '手动选择'
})

const formatLastUpdate = computed(() => {
  if (!weatherStore.weatherData?.lastUpdated) return ''
  const date = new Date(weatherStore.weatherData.lastUpdated)
  return date.toLocaleString('zh-CN')
})

// Columns
const locationTypeColumns = [
  { text: '自动定位', value: 'auto' },
  { text: '手动选择', value: 'manual' },
]

const intervalColumns = [
  { text: '15分钟', value: 15 },
  { text: '30分钟', value: 30 },
  { text: '1小时', value: 60 },
  { text: '2小时', value: 120 },
]

// Methods
const getWeatherIcon = (code: number) => {
  const iconMap: Record<number, string> = {
    1000: '☀️', 1003: '⛅', 1006: '☁️', 1009: '🌫️',
    1030: '🌫️', 1063: '🌧️', 1066: '🌨️', 1069: '🌨️',
    1072: '🌧️', 1087: '⛈️', 1114: '🌨️', 1117: '🌨️',
    1135: '🌫️', 1147: '🌫️', 1150: '🌧️', 1153: '🌧️',
    1168: '🌧️', 1171: '🌧️', 1180: '🌧️', 1183: '🌧️',
    1186: '🌧️', 1189: '🌧️', 1192: '🌧️', 1195: '🌧️',
    1198: '🌧️', 1201: '🌧️', 1204: '🌨️', 1207: '🌨️',
    1210: '🌨️', 1213: '🌨️', 1216: '🌨️', 1219: '🌨️',
    1222: '🌨️', 1225: '🌨️', 1237: '🌨️', 1240: '🌧️',
    1243: '🌧️', 1246: '🌧️', 1249: '🌨️', 1252: '🌨️',
    1255: '🌨️', 1258: '🌨️', 1261: '🌨️', 1264: '🌨️',
    1273: '⛈️', 1276: '⛈️', 1279: '⛈️', 1282: '⛈️',
  }
  return iconMap[code] || '🌡️'
}

const formatDate = (dateStr: string) => {
  const date = new Date(dateStr)
  const today = new Date()
  const tomorrow = new Date(today)
  tomorrow.setDate(tomorrow.getDate() + 1)
  
  if (date.toDateString() === today.toDateString()) return '今天'
  if (date.toDateString() === tomorrow.toDateString()) return '明天'
  return date.toLocaleDateString('zh-CN', { month: 'short', day: 'numeric' })
}

const onToggleEnabled = () => {
  weatherStore.saveSettings()
  if (weatherStore.settings.enabled) {
    weatherStore.fetchWeather()
  }
}

const detectLocation = () => {
  weatherStore.detectLocation()
}

const onLocationTypeConfirm = ({ selectedOptions }: any) => {
  weatherStore.updateSettings({ locationType: selectedOptions[0].value })
  showLocationTypePicker.value = false
  saveSettings()
}

const onIntervalConfirm = ({ selectedOptions }: any) => {
  weatherStore.updateSettings({ updateInterval: selectedOptions[0].value })
  showIntervalPicker.value = false
  saveSettings()
}

const toggleUnit = () => {
  weatherStore.toggleTemperatureUnit()
  saveSettings()
}

const searchLocation = () => {
  // TODO: 调用API搜索城市
  showToast('搜索功能开发中')
}

const selectLocation = (city: Location) => {
  weatherStore.updateSettings({ location: city })
  showLocationSearch.value = false
  weatherStore.fetchWeather()
  saveSettings()
}

const saveSettings = () => {
  weatherStore.saveSettings()
}

onMounted(() => {
  if (weatherStore.settings.enabled) {
    weatherStore.fetchWeather()
  }
})
</script>

<style scoped>
.weather-settings-page {
  padding-bottom: 20px;
}

.weather-card {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  margin: 12px;
  border-radius: 12px;
  padding: 24px;
  color: white;
}

.weather-main {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16px;
}

.weather-icon {
  font-size: 64px;
}

.weather-temp {
  display: flex;
  align-items: flex-start;
}

.temp-value {
  font-size: 72px;
  font-weight: 300;
  line-height: 1;
}

.temp-unit {
  font-size: 24px;
  margin-top: 8px;
}

.weather-desc {
  text-align: center;
  font-size: 18px;
  margin-top: 8px;
  opacity: 0.9;
}

.weather-details {
  display: flex;
  justify-content: space-around;
  margin-top: 24px;
  padding-top: 16px;
  border-top: 1px solid rgba(255, 255, 255, 0.2);
}

.detail-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  opacity: 0.9;
}

.weather-location {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  margin-top: 16px;
  font-size: 14px;
  opacity: 0.8;
}

.mt-12 {
  margin-top: 12px;
}

.forecast-list {
  padding: 8px 0;
}

.forecast-item {
  display: flex;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid #f5f5f5;
}

.forecast-item:last-child {
  border-bottom: none;
}

.forecast-date {
  width: 60px;
  font-size: 14px;
  color: #666;
}

.forecast-icon {
  width: 40px;
  text-align: center;
  font-size: 24px;
}

.forecast-temp {
  width: 80px;
  text-align: center;
  font-size: 14px;
  color: #323233;
}

.forecast-condition {
  flex: 1;
  text-align: right;
  font-size: 14px;
  color: #666;
}

.last-update {
  text-align: center;
  padding: 16px;
  font-size: 12px;
  color: #999;
}

.location-search {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.location-list {
  flex: 1;
  overflow-y: auto;
}
</style>