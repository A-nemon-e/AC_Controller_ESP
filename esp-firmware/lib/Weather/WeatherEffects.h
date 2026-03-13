#ifndef WEATHER_EFFECTS_H
#define WEATHER_EFFECTS_H

#include <Arduino.h>
#include "WeatherModule.h"
#include "../Background.h"

class WeatherEffects {
public:
  static void renderSunny(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderCloudy(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderOvercast(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderRain(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderSnow(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderWind(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  static void renderHot(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);
  
  static void renderByCondition(WeatherCondition condition, LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount);

private:
  // Rain drop positions
  static uint8_t rainDrops[10];
  static uint8_t snowFlakes[10][2];
  static uint32_t lastUpdate;
  
  static void initParticles();
  static void updateRain(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
  static void updateSnow(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
};

#endif