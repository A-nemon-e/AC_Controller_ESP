export interface CardConfig {
  id: string
  name: string
  enabled: boolean
  duration: number
  background: BackgroundConfig | null
  foregrounds: ForegroundConfig[]
}

export interface BackgroundConfig {
  type: string
  brightness: number
  color?: string
  gradientColors?: string[]
}

export interface ForegroundConfig {
  type: string
  position: string
  font: string
  color?: string
  brightness: number
  enabled: boolean
  suffix?: string
}

export interface DisplaySettings {
  brightness: number
  autoSwitch: boolean
  switchInterval: number
  transition: string
  transitionSpeed: number
  nightMode: boolean
  nightModeStart: string
  nightModeEnd: string
  nightModeBrightness: number
}

export interface WeatherOverlay {
  enabled: boolean
  showIcon: boolean
  showTemperature: boolean
  position: string
}

export interface DisplayCard {
  id: number
  name: string
  background: BackgroundConfig | null
  foregrounds: ForegroundConfig[]
  autoSwitchForegrounds: boolean
}
