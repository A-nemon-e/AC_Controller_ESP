#include "WeatherEffects.h"

uint8_t WeatherEffects::rainDrops[10] = {0};
uint8_t WeatherEffects::snowFlakes[10][2] = {{0}};
uint32_t WeatherEffects::lastUpdate = 0;

void WeatherEffects::initParticles() {
  if (lastUpdate == 0) {
    for (int i = 0; i < 10; i++) {
      rainDrops[i] = random(32);
      snowFlakes[i][0] = random(32);
      snowFlakes[i][1] = random(32);
    }
    lastUpdate = millis();
  }
}

void WeatherEffects::renderByCondition(WeatherCondition condition, LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  switch (condition) {
    case WEATHER_SUNNY:
      renderSunny(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_CLOUDY:
      renderCloudy(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_OVERCAST:
      renderOvercast(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_RAIN:
      renderRain(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_SNOW:
      renderSnow(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_WIND:
      renderWind(matrix, x, y, width, height, frameCount);
      break;
    case WEATHER_HOT:
      renderHot(matrix, x, y, width, height, frameCount);
      break;
    default:
      renderSunny(matrix, x, y, width, height, frameCount);
  }
}

void WeatherEffects::renderSunny(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  uint8_t cx = x + width / 2;
  uint8_t cy = y + height / 2;
  
  // Sun center
  for (int8_t dy = -2; dy <= 2; dy++) {
    for (int8_t dx = -2; dx <= 2; dx++) {
      if (dx*dx + dy*dy <= 5) {
        matrix.setPixel(cx + dx, cy + dy, 0xFFA500);
      }
    }
  }
  
  // Rotating rays
  uint8_t numRays = 8;
  float angle = (frameCount * 0.05f);
  
  for (uint8_t i = 0; i < numRays; i++) {
    float rayAngle = angle + (i * 2 * PI / numRays);
    for (uint8_t r = 4; r <= 7; r++) {
      int8_t rx = cx + cos(rayAngle) * r;
      int8_t ry = cy + sin(rayAngle) * r;
      if (rx >= x && rx < x + width && ry >= y && ry < y + height) {
        matrix.setPixel(rx, ry, 0xFFD700);
      }
    }
  }
}

void WeatherEffects::renderCloudy(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  int8_t offset = (frameCount / 20) % (width + 4) - 2;
  
  // Draw clouds
  uint8_t cloudY = y + height / 3;
  
  // Cloud 1
  for (int8_t dx = -3; dx <= 3; dx++) {
    for (int8_t dy = -1; dy <= 1; dy++) {
      if (abs(dx) + abs(dy) <= 3) {
        int8_t px = x + width / 3 + dx + offset;
        int8_t py = cloudY + dy;
        if (px >= x && px < x + width) {
          matrix.setPixel(px, py, 0xC0C0C0);
        }
      }
    }
  }
  
  // Cloud 2 (slightly offset)
  for (int8_t dx = -2; dx <= 2; dx++) {
    for (int8_t dy = -1; dy <= 1; dy++) {
      if (abs(dx) + abs(dy) <= 2) {
        int8_t px = x + 2 * width / 3 + dx + offset - 1;
        int8_t py = cloudY + 1 + dy;
        if (px >= x && px < x + width) {
          matrix.setPixel(px, py, 0xD3D3D3);
        }
      }
    }
  }
}

void WeatherEffects::renderOvercast(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  // Gray gradient background
  for (uint8_t py = y; py < y + height; py++) {
    uint8_t gray = 0x40 + (py - y) * 0x10 / height;
    for (uint8_t px = x; px < x + width; px++) {
      matrix.setPixel(px, py, (gray << 16) | (gray << 8) | gray);
    }
  }
}

void WeatherEffects::renderRain(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  initParticles();
  
  // Dark clouds at top
  for (uint8_t py = y; py < y + 3; py++) {
    for (uint8_t px = x; px < x + width; px++) {
      matrix.setPixel(px, py, 0x404040);
    }
  }
  
  // Falling rain drops
  if (frameCount % 3 == 0) {
    updateRain(matrix, x, y + 3, width, height - 3);
  }
  
  // Render rain drops
  for (int i = 0; i < 10; i++) {
    uint8_t dropY = rainDrops[i];
    uint8_t dropX = x + (i * width / 10);
    if (dropY >= y + 3 && dropY < y + height) {
      matrix.setPixel(dropX, dropY, 0x0080FF);
      if (dropY > y + 3) {
        matrix.setPixel(dropX, dropY - 1, 0x0040FF);
      }
    }
  }
}

void WeatherEffects::updateRain(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  for (int i = 0; i < 10; i++) {
    rainDrops[i] += 2;
    if (rainDrops[i] >= y + height) {
      rainDrops[i] = y;
    }
  }
}

void WeatherEffects::renderSnow(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  initParticles();
  
  // Light blue/white background
  for (uint8_t py = y; py < y + height; py++) {
    for (uint8_t px = x; px < x + width; px++) {
      matrix.setPixel(px, py, 0xE0E8FF);
    }
  }
  
  // Update snowflakes
  if (frameCount % 5 == 0) {
    updateSnow(matrix, x, y, width, height);
  }
  
  // Render snowflakes
  for (int i = 0; i < 10; i++) {
    uint8_t sx = snowFlakes[i][0];
    uint8_t sy = snowFlakes[i][1];
    if (sx >= x && sx < x + width && sy >= y && sy < y + height) {
      matrix.setPixel(sx, sy, 0xFFFFFF);
    }
  }
}

void WeatherEffects::updateSnow(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  for (int i = 0; i < 10; i++) {
    snowFlakes[i][1]++;
    // Drift left or right
    snowFlakes[i][0] += (i % 2 == 0) ? 1 : -1;
    
    if (snowFlakes[i][1] >= y + height) {
      snowFlakes[i][1] = y;
      snowFlakes[i][0] = x + random(width);
    }
    if (snowFlakes[i][0] < x) snowFlakes[i][0] = x + width - 1;
    if (snowFlakes[i][0] >= x + width) snowFlakes[i][0] = x;
  }
}

void WeatherEffects::renderWind(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  uint8_t offset = (frameCount / 5) % (width + 8);
  
  // Draw wind lines
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t lineY = y + 2 + i * 3;
    uint8_t lineOffset = (offset + i * 4) % (width + 8);
    
    for (uint8_t px = 0; px < 6; px++) {
      int8_t drawX = x + lineOffset - 4 + px;
      if (drawX >= x && drawX < x + width) {
        uint8_t brightness = 0x60 + px * 0x10;
        matrix.setPixel(drawX, lineY, (brightness << 16) | (brightness << 8) | brightness);
      }
    }
  }
}

void WeatherEffects::renderHot(LEDMatrix& matrix, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t frameCount) {
  // Fire effect at bottom
  uint8_t fireHeight = 5;
  
  for (uint8_t py = y + height - fireHeight; py < y + height; py++) {
    for (uint8_t px = x; px < x + width; px++) {
      uint8_t flameIntensity = random(0x80, 0xFF);
      uint8_t distanceFromBottom = y + height - py;
      uint8_t red = flameIntensity;
      uint8_t green = flameIntensity * (fireHeight - distanceFromBottom) / fireHeight;
      
      matrix.setPixel(px, py, (red << 16) | (green << 8));
    }
  }
  
  // Flickering effect
  if (frameCount % 4 == 0) {
    uint8_t flickerX = x + random(width);
    uint8_t flickerY = y + height - fireHeight + random(fireHeight);
    matrix.setPixel(flickerX, flickerY, 0xFFFF00);
  }
}