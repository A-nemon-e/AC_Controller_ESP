import apiClient from './client'
import type { WeatherSettings } from '@/types/weather'

export const weatherApi = {
  getWeather: (deviceId: string) =>
    apiClient.get(`/devices/${deviceId}/weather`),
  
  getSettings: (deviceId: string) =>
    apiClient.get(`/devices/${deviceId}/weather-settings`),
  
  saveSettings: (deviceId: string, settings: WeatherSettings) =>
    apiClient.post(`/devices/${deviceId}/weather-settings`, settings),
  
  searchLocation: (query: string) =>
    apiClient.get('/weather/locations', { params: { q: query } }),
}
