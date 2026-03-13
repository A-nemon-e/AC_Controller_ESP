#ifndef WEATHER_MODULE_H
#define WEATHER_MODULE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

enum WeatherCondition {
  WEATHER_SUNNY = 0,
  WEATHER_CLOUDY,
  WEATHER_OVERCAST,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_WIND,
  WEATHER_FOG,
  WEATHER_HOT,
  WEATHER_UNKNOWN
};

struct WeatherData {
  float temperature;
  uint8_t humidity;
  String condition;
  WeatherCondition conditionCode;
  uint8_t windSpeed;
  String windDirection;
  uint16_t aqi;
  bool isValid;
  unsigned long receivedTime;
};

class WeatherModule {
public:
  WeatherModule();
  
  void begin();
  void update();
  
  bool parseWeatherData(const String& jsonData);
  
  const WeatherData& getCurrentWeather() const { return currentWeather; }
  WeatherCondition getConditionCode() const { return currentWeather.conditionCode; }
  
  bool isWeatherValid() const;
  bool isHighTemperature() const { return currentWeather.temperature >= 30; }
  bool isLowTemperature() const { return currentWeather.temperature < 0; }
  
  void setOnWeatherUpdateCallback(std::function<void(const WeatherData&)> callback) {
    onWeatherUpdate = callback;
  }
  
  static String getConditionName(WeatherCondition condition);
  static uint16_t getConditionColor(WeatherCondition condition);

private:
  WeatherData currentWeather;
  std::function<void(const WeatherData&)> onWeatherUpdate;
  
  WeatherCondition parseConditionCode(const String& code);
  void invalidateWeather();
};

#endif