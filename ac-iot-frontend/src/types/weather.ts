export interface WeatherData {
  conditionCode: string
  temperature: number
  humidity: number
  windSpeed: number
  windDirection: string
  pressure?: number
  visibility?: number
}

export interface WeatherSettings {
  enabled: boolean
  locationMode: 'auto' | 'manual'
  city?: string
  updateInterval: number
  unit: 'celsius' | 'fahrenheit'
  showForecast?: boolean
  displayEffects?: boolean
  temperatureUnit?: 'celsius' | 'fahrenheit'
  locationType?: 'auto' | 'manual'
  location?: Location
  apiKey?: string
}

export interface ForecastDay {
  date: string
  conditionCode: string
  tempMax: number
  tempMin: number
}

export interface Location {
  name: string
  region?: string
  country?: string
  lat?: number
  lon?: number
}

export interface NestedWeatherData {
  location: {
    name: string
    region: string
    country: string
  }
  current: {
    temperature: number
    feelsLike?: number
    humidity: number
    pressure?: number
    windSpeed: number
    windDirection: string
    condition: {
      code: number
      text: string
      icon?: string
    }
    uvIndex?: number
    visibility?: number
  }
  forecast?: Array<{
    date: string
    maxTemp: number
    minTemp: number
    condition: {
      code: number
      text: string
      icon?: string
    }
    precipitation?: number
    humidity?: number
  }>
  lastUpdated?: string
}
