#include "WeatherModule.h"

WeatherModule::WeatherModule() {
  invalidateWeather();
}

void WeatherModule::begin() {
  invalidateWeather();
}

void WeatherModule::update() {
  // Check if weather data has expired (30 minutes)
  if (currentWeather.isValid && 
      (millis() - currentWeather.receivedTime > 30 * 60 * 1000)) {
    invalidateWeather();
  }
}

bool WeatherModule::parseWeatherData(const String& jsonData) {
  StaticJsonDocument<1024> doc;
  
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print("Weather JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Check message type
  const char* type = doc["type"];
  if (!type || strcmp(type, "weather_update") != 0) {
    return false;
  }
  
  JsonObject data = doc["data"];
  if (data.isNull()) {
    return false;
  }
  
  // Parse weather data
  currentWeather.temperature = data["temperature"] | 0.0f;
  currentWeather.humidity = data["humidity"] | 0;
  currentWeather.condition = data["condition"] | "Unknown";
  currentWeather.windSpeed = data["windSpeed"] | 0;
  currentWeather.windDirection = data["windDirection"] | "";
  currentWeather.aqi = data["aqi"] | 0;
  currentWeather.receivedTime = millis();
  currentWeather.isValid = true;
  
  // Parse condition code
  const char* code = data["conditionCode"];
  if (code) {
    currentWeather.conditionCode = parseConditionCode(String(code));
  } else {
    currentWeather.conditionCode = WEATHER_UNKNOWN;
  }
  
  // Special handling for high temperature (30°C and above)
  if (currentWeather.temperature >= 30) {
    currentWeather.conditionCode = WEATHER_HOT;
  }
  
  // Trigger callback
  if (onWeatherUpdate) {
    onWeatherUpdate(currentWeather);
  }
  
  return true;
}

WeatherCondition WeatherModule::parseConditionCode(const String& code) {
  if (code == "sunny") return WEATHER_SUNNY;
  if (code == "cloudy") return WEATHER_CLOUDY;
  if (code == "overcast") return WEATHER_OVERCAST;
  if (code == "rain") return WEATHER_RAIN;
  if (code == "snow") return WEATHER_SNOW;
  if (code == "wind") return WEATHER_WIND;
  if (code == "fog") return WEATHER_FOG;
  if (code == "hot") return WEATHER_HOT;
  // 添加更多天气代码映射，匹配和风天气API
  if (code == "100") return WEATHER_SUNNY;  // 晴
  if (code == "101" || code == "102" || code == "103") return WEATHER_CLOUDY;  // 多云/少云/晴间多云
  if (code == "104") return WEATHER_OVERCAST;  // 阴
  if (code >= "300" && code <= "399") return WEATHER_RAIN;  // 雨
  if (code >= "400" && code <= "499") return WEATHER_SNOW;  // 雪
  if (code >= "500" && code <= "599") return WEATHER_WIND;  // 风
  return WEATHER_UNKNOWN;
}

bool WeatherModule::isWeatherValid() const {
  if (!currentWeather.isValid) return false;
  
  // Check if data is older than 30 minutes
  if (millis() - currentWeather.receivedTime > 30 * 60 * 1000) {
    return false;
  }
  
  return true;
}

void WeatherModule::invalidateWeather() {
  currentWeather.isValid = false;
  currentWeather.temperature = 0;
  currentWeather.humidity = 0;
  currentWeather.windSpeed = 0;
  currentWeather.aqi = 0;
  currentWeather.conditionCode = WEATHER_UNKNOWN;
}

String WeatherModule::getConditionName(WeatherCondition condition) {
  switch (condition) {
    case WEATHER_SUNNY: return "晴";
    case WEATHER_CLOUDY: return "多云";
    case WEATHER_OVERCAST: return "阴";
    case WEATHER_RAIN: return "雨";
    case WEATHER_SNOW: return "雪";
    case WEATHER_WIND: return "风";
    case WEATHER_FOG: return "雾";
    case WEATHER_HOT: return "高温";
    default: return "未知";
  }
}

uint16_t WeatherModule::getConditionColor(WeatherCondition condition) {
  switch (condition) {
    case WEATHER_SUNNY: return 0xFFA500;  // Orange
    case WEATHER_CLOUDY: return 0xC0C0C0; // Silver
    case WEATHER_OVERCAST: return 0x808080; // Gray
    case WEATHER_RAIN: return 0x0080FF;   // Blue
    case WEATHER_SNOW: return 0xFFFFFF;   // White
    case WEATHER_WIND: return 0x00FF80;   // Green
    case WEATHER_FOG: return 0xA9A9A9;    // DarkGray
    case WEATHER_HOT: return 0xFF0000;    // Red
    default: return 0xFFFFFF;
  }
}