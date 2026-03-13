import apiClient from './client'
import type { DisplaySettings, CardConfig } from '@/types/display'

export const displayApi = {
  getSettings: (deviceId: string) => 
    apiClient.get(`/devices/${deviceId}/display-settings`),
  
  saveSettings: (deviceId: string, settings: DisplaySettings) =>
    apiClient.post(`/devices/${deviceId}/display-settings`, settings),
  
  getCards: (deviceId: string) =>
    apiClient.get(`/devices/${deviceId}/cards`),
  
  saveCards: (deviceId: string, cards: CardConfig[]) =>
    apiClient.post(`/devices/${deviceId}/cards`, cards),
}
