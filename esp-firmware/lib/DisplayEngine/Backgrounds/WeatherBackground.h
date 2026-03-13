/**
 * WeatherBackground - 天气背景系统
 * 
 * 支持多种天气效果：晴、雨、雪、风、阴
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef WEATHER_BACKGROUND_H
#define WEATHER_BACKGROUND_H

#include "../Backgrounds/Background.h"

class WeatherBackground : public Background {
public:
    enum class WeatherType {
        SUNNY,      // 晴天（太阳）
        RAINY,      // 下雨
        SNOWY,      // 下雪
        WINDY,      // 刮风
        CLOUDY      // 阴天
    };
    
    WeatherBackground() : Background(Type::WEATHER_SUNNY), 
                         currentType(WeatherType::SUNNY),
                         rainLevel(1),
                         lastRainChange(0),
                         sunAngle(0) {
        updateInterval = 50;
        memset(raindrops, 0, sizeof(raindrops));
        memset(snowflakes, 0, sizeof(snowflakes));
        memset(windLines, 0, sizeof(windLines));
        memset(clouds, 0, sizeof(clouds));
    }
    
    void init() override {
        Background::init();
        initWeather(currentType);
    }
    
    void setWeather(WeatherType type) {
        currentType = type;
        initWeather(type);
    }
    
    WeatherType getWeather() const { return currentType; }
    
    void render(LEDMatrix& matrix) override {
        matrix.clear();
        
        switch (currentType) {
            case WeatherType::SUNNY:
                renderSunny(matrix);
                break;
            case WeatherType::RAINY:
                renderRainy(matrix);
                break;
            case WeatherType::SNOWY:
                renderSnowy(matrix);
                break;
            case WeatherType::WINDY:
                renderWindy(matrix);
                break;
            case WeatherType::CLOUDY:
                renderCloudy(matrix);
                break;
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            switch (currentType) {
                case WeatherType::SUNNY:
                    updateSunny();
                    break;
                case WeatherType::RAINY:
                    updateRainy();
                    break;
                case WeatherType::SNOWY:
                    updateSnowy();
                    break;
                case WeatherType::WINDY:
                    updateWindy();
                    break;
                case WeatherType::CLOUDY:
                    updateCloudy();
                    break;
            }
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    WeatherType currentType;
    uint8_t rainLevel;  // 0=小, 1=中, 2=大
    uint32_t lastRainChange;
    
    // 雨滴
    struct RainDrop {
        int16_t x, y;
        uint8_t speed;
        bool active;
    } raindrops[20];
    
    // 雪花
    struct SnowFlake {
        int16_t x, y;
        uint8_t speed;
        int8_t drift;
        bool active;
    } snowflakes[15];
    
    // 风线
    struct WindLine {
        int16_t x, y;
        uint8_t length;
        uint8_t speed;
        bool active;
    } windLines[10];
    
    // 云朵
    struct Cloud {
        int16_t x, y;
        uint8_t width;
        bool active;
    } clouds[5];
    
    // 太阳
    uint8_t sunAngle;
    
    void initWeather(WeatherType type) {
        switch (type) {
            case WeatherType::RAINY:
                for (uint8_t i = 0; i < 20; i++) {
                    raindrops[i].active = false;
                }
                break;
            case WeatherType::SNOWY:
                for (uint8_t i = 0; i < 15; i++) {
                    snowflakes[i].active = false;
                }
                break;
            case WeatherType::WINDY:
                for (uint8_t i = 0; i < 10; i++) {
                    windLines[i].active = false;
                }
                break;
            case WeatherType::CLOUDY:
                for (uint8_t i = 0; i < 5; i++) {
                    clouds[i].active = false;
                }
                break;
            default:
                break;
        }
    }
    
    // 晴天渲染
    void renderSunny(LEDMatrix& matrix) {
        // 绘制太阳（旋转的十字）
        uint8_t cx = SCREEN_COLS - 5;
        uint8_t cy = 3;
        
        // 中心
        matrix.setPixel(cx, cy, brightness);
        
        // 旋转的光芒
        uint8_t radius = 2;
        for (uint8_t i = 0; i < 4; i++) {
            float angle = (sunAngle + i * 90) * PI / 180;
            int8_t dx = (int8_t)(cos(angle) * radius);
            int8_t dy = (int8_t)(sin(angle) * radius);
            matrix.setPixelClipped(cx + dx, cy + dy, brightness / 2);
        }
    }
    
    void updateSunny() {
        sunAngle += 5;
        if (sunAngle >= 360) sunAngle = 0;
    }
    
    // 雨天渲染
    void renderRainy(LEDMatrix& matrix) {
        uint8_t maxDrops = (rainLevel + 1) * 7;  // 7, 14, 21
        
        for (uint8_t i = 0; i < maxDrops; i++) {
            if (raindrops[i].active && 
                raindrops[i].x >= 0 && raindrops[i].x < SCREEN_COLS &&
                raindrops[i].y >= 0 && raindrops[i].y < SCREEN_ROWS) {
                matrix.setPixel((uint8_t)raindrops[i].x, (uint8_t)raindrops[i].y, brightness);
            }
        }
    }
    
    void updateRainy() {
        uint8_t maxDrops = (rainLevel + 1) * 7;
        
        // 更新雨滴
        for (uint8_t i = 0; i < maxDrops; i++) {
            if (raindrops[i].active) {
                raindrops[i].y += raindrops[i].speed;
                if (raindrops[i].y >= SCREEN_ROWS) {
                    raindrops[i].active = false;
                }
            } else if (random(100) < 20) {
                // 生成新雨滴
                raindrops[i].x = random(SCREEN_COLS);
                raindrops[i].y = 0;
                raindrops[i].speed = random(2, 4 + rainLevel);
                raindrops[i].active = true;
            }
        }
        
        // 偶尔改变雨的大小
        if (millis() - lastRainChange > 10000 && random(100) < 5) {
            rainLevel = random(3);
            lastRainChange = millis();
        }
    }
    
    // 雪天渲染
    void renderSnowy(LEDMatrix& matrix) {
        for (uint8_t i = 0; i < 15; i++) {
            if (snowflakes[i].active &&
                snowflakes[i].x >= 0 && snowflakes[i].x < SCREEN_COLS &&
                snowflakes[i].y >= 0 && snowflakes[i].y < SCREEN_ROWS) {
                matrix.setPixel((uint8_t)snowflakes[i].x, (uint8_t)snowflakes[i].y, brightness);
            }
        }
    }
    
    void updateSnowy() {
        for (uint8_t i = 0; i < 15; i++) {
            if (snowflakes[i].active) {
                snowflakes[i].y += snowflakes[i].speed;
                snowflakes[i].x += snowflakes[i].drift;
                
                if (snowflakes[i].y >= SCREEN_ROWS || 
                    snowflakes[i].x < 0 || 
                    snowflakes[i].x >= SCREEN_COLS) {
                    snowflakes[i].active = false;
                }
            } else if (random(100) < 15) {
                snowflakes[i].x = random(SCREEN_COLS);
                snowflakes[i].y = 0;
                snowflakes[i].speed = random(1, 3);
                snowflakes[i].drift = random(-1, 2);
                snowflakes[i].active = true;
            }
        }
    }
    
    // 风天渲染
    void renderWindy(LEDMatrix& matrix) {
        for (uint8_t i = 0; i < 10; i++) {
            if (windLines[i].active) {
                for (uint8_t l = 0; l < windLines[i].length; l++) {
                    int16_t px = windLines[i].x - l;
                    if (px >= 0 && px < SCREEN_COLS &&
                        windLines[i].y >= 0 && windLines[i].y < SCREEN_ROWS) {
                        uint8_t bri = brightness / (l + 1);
                        matrix.setPixel((uint8_t)px, (uint8_t)windLines[i].y, bri);
                    }
                }
            }
        }
    }
    
    void updateWindy() {
        for (uint8_t i = 0; i < 10; i++) {
            if (windLines[i].active) {
                windLines[i].x += windLines[i].speed;
                if (windLines[i].x - windLines[i].length >= SCREEN_COLS) {
                    windLines[i].active = false;
                }
            } else if (random(100) < 10) {
                windLines[i].x = 0;
                windLines[i].y = random(SCREEN_ROWS);
                windLines[i].length = random(3, 8);
                windLines[i].speed = random(2, 5);
                windLines[i].active = true;
            }
        }
    }
    
    // 阴天渲染
    void renderCloudy(LEDMatrix& matrix) {
        for (uint8_t i = 0; i < 5; i++) {
            if (clouds[i].active) {
                // 绘制云朵（简单的矩形表示）
                for (uint8_t w = 0; w < clouds[i].width; w++) {
                    for (uint8_t h = 0; h < 2; h++) {
                        int16_t px = clouds[i].x + w;
                        int16_t py = clouds[i].y + h;
                        if (px >= 0 && px < SCREEN_COLS && py >= 0 && py < SCREEN_ROWS) {
                            matrix.setPixel((uint8_t)px, (uint8_t)py, brightness / 2);
                        }
                    }
                }
            }
        }
    }
    
    void updateCloudy() {
        for (uint8_t i = 0; i < 5; i++) {
            if (clouds[i].active) {
                clouds[i].x += 1;
                if (clouds[i].x >= SCREEN_COLS) {
                    clouds[i].active = false;
                }
            } else if (random(100) < 8) {
                clouds[i].x = -random(5, 10);
                clouds[i].y = random(SCREEN_ROWS - 2);
                clouds[i].width = random(4, 8);
                clouds[i].active = true;
            }
        }
    }
};

#endif // WEATHER_BACKGROUND_H
