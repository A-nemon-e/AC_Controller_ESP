import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { NestedWeatherData, WeatherSettings } from '@/types/weather'
import { weatherApi } from '@/api/weather'

export const useWeatherStore = defineStore('weather', () => {
    // State
    const settings = ref<WeatherSettings>({
        enabled: true,
        apiKey: '',
        locationType: 'auto',
        location: {
            name: '北京',
            region: 'Beijing',
            country: 'China',
            lat: 39.9042,
            lon: 116.4074,
        },
        updateInterval: 30,
        showForecast: true,
        temperatureUnit: 'celsius',
        displayEffects: true,
        unit: 'celsius',
        locationMode: 'auto',
    })

    const weatherData = ref<NestedWeatherData | null>({
        location: {
            name: '北京',
            region: 'Beijing',
            country: 'China',
        },
        current: {
            temperature: 25,
            feelsLike: 27,
            humidity: 60,
            pressure: 1013,
            windSpeed: 3.5,
            windDirection: 'SE',
            condition: {
                code: 1000,
                text: '晴朗',
                icon: 'sunny',
            },
            uvIndex: 5,
            visibility: 10,
        },
        forecast: [
            {
                date: '2024-03-14',
                maxTemp: 28,
                minTemp: 18,
                condition: {
                    code: 1000,
                    text: '晴朗',
                    icon: 'sunny',
                },
                precipitation: 0,
                humidity: 55,
            },
            {
                date: '2024-03-15',
                maxTemp: 26,
                minTemp: 16,
                condition: {
                    code: 1003,
                    text: '多云',
                    icon: 'cloudy',
                },
                precipitation: 10,
                humidity: 65,
            },
        ],
        lastUpdated: new Date().toISOString(),
    })

    const loading = ref(false)
    const error = ref<string | null>(null)
    const lastFetchTime = ref<Date | null>(null)
    const currentDeviceId = ref<string>('')

    // Getters
    const currentTemperature = computed(() => {
        if (!weatherData.value) return null
        const temp = weatherData.value.current.temperature
        if (settings.value.temperatureUnit === 'fahrenheit') {
            return Math.round(temp * 9 / 5 + 32)
        }
        return Math.round(temp)
    })

    const temperatureUnit = computed(() => {
        return settings.value.temperatureUnit === 'celsius' ? '°C' : '°F'
    })

    const formattedLocation = computed(() => {
        if (!weatherData.value?.location) return ''
        const { name, region, country } = weatherData.value.location
        return `${name}${region ? `, ${region}` : ''}${country ? `, ${country}` : ''}`
    })

    const isEnabled = computed(() => settings.value.enabled)

    const currentLocation = computed(() => {
        return settings.value.location?.name || ''
    })

    const lastUpdate = computed(() => {
        return weatherData.value?.lastUpdated || null
    })

    const isStale = computed(() => {
        if (!lastFetchTime.value) return true
        const now = new Date()
        const diff = now.getTime() - lastFetchTime.value.getTime()
        const minutes = settings.value.updateInterval
        return diff > minutes * 60 * 1000
    })

    // Actions
    const updateSettings = (newSettings: Partial<WeatherSettings>) => {
        settings.value = { ...settings.value, ...newSettings }
    }

    const setLocation = (lat: number, lon: number, name?: string) => {
        settings.value.location = {
            ...settings.value.location,
            lat,
            lon,
            name: name || settings.value.location?.name || '',
        }
    }

    const fetchWeather = async (deviceId?: string) => {
        if (!settings.value.enabled) return
        loading.value = true
        error.value = null

        try {
            const targetDeviceId = deviceId || currentDeviceId.value
            if (!targetDeviceId) {
                console.warn('No device ID provided for fetchWeather')
                return
            }
            
            const response = await weatherApi.getWeather(targetDeviceId)
            if (response.data) {
                weatherData.value = response.data
            }
            lastFetchTime.value = new Date()
        } catch (e) {
            error.value = e instanceof Error ? e.message : '获取天气失败'
            console.error('Failed to fetch weather:', e)
        } finally {
            loading.value = false
        }
    }

    const detectLocation = async () => {
        if (!navigator.geolocation) {
            error.value = '浏览器不支持地理定位'
            return
        }

        loading.value = true
        try {
            const position = await new Promise<GeolocationPosition>((resolve, reject) => {
                navigator.geolocation.getCurrentPosition(resolve, reject)
            })
            setLocation(
                position.coords.latitude,
                position.coords.longitude
            )
            await fetchWeather()
        } catch (e) {
            error.value = e instanceof Error ? e.message : '定位失败'
            console.error('Failed to detect location:', e)
        } finally {
            loading.value = false
        }
    }

    const saveSettings = async (deviceId?: string) => {
        loading.value = true
        try {
            const targetDeviceId = deviceId || currentDeviceId.value
            if (!targetDeviceId) {
                console.warn('No device ID provided for saveSettings')
                return
            }
            
            await weatherApi.saveSettings(targetDeviceId, settings.value)
        } catch (error) {
            console.error('Failed to save weather settings:', error)
            throw error
        } finally {
            loading.value = false
        }
    }

    const toggleEnabled = () => {
        settings.value.enabled = !settings.value.enabled
    }

    const toggleTemperatureUnit = () => {
        settings.value.temperatureUnit = 
            settings.value.temperatureUnit === 'celsius' ? 'fahrenheit' : 'celsius'
    }

    const setManualLocation = async (city: string) => {
        settings.value.locationMode = 'manual'
        settings.value.location = {
            ...settings.value.location,
            name: city,
        }
    }

    const refreshWeather = async () => {
        await fetchWeather()
    }

    const setDeviceId = (deviceId: string) => {
        currentDeviceId.value = deviceId
    }

    return {
        settings,
        weatherData,
        loading,
        error,
        lastFetchTime,
        currentTemperature,
        temperatureUnit,
        formattedLocation,
        isEnabled,
        currentLocation,
        lastUpdate,
        isStale,
        updateSettings,
        setLocation,
        fetchWeather,
        detectLocation,
        saveSettings,
        toggleEnabled,
        toggleTemperatureUnit,
        setManualLocation,
        refreshWeather,
        setDeviceId,
    }
})
