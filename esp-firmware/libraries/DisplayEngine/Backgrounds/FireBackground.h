/**
 * FireBackground - 火焰效果背景
 * 
 * 动态火焰动画效果
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef FIRE_BACKGROUND_H
#define FIRE_BACKGROUND_H

#include "../Backgrounds/Background.h"

class FireBackground : public Background {
public:
    FireBackground() : Background(Type::FIRE) {
        updateInterval = 50;  // 20fps
        memset(firePixels, 0, sizeof(firePixels));
    }
    
    void init() override {
        Background::init();
        memset(firePixels, 0, sizeof(firePixels));
    }
    
    void render(LEDMatrix& matrix) override {
        // 1. 更新火焰算法
        updateFire();
        
        // 2. 渲染到矩阵
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint8_t intensity = firePixels[y][x];
                // 映射到LED亮度（0-255）
                matrix.setPixel(x, y, (uint16_t)intensity * brightness / 255);
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            updateFire();
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, false, false, false, false, Position::TOP_CENTER};
    }

private:
    uint8_t firePixels[SCREEN_ROWS][SCREEN_COLS];
    
    void updateFire() {
        // 火焰算法
        // 1. 底部生成火种
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            firePixels[SCREEN_ROWS-1][x] = random(160, 255);
        }
        
        // 2. 向上传播并衰减
        for (int8_t y = SCREEN_ROWS - 2; y >= 0; y--) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                // 从下方像素获取热量
                uint8_t below = firePixels[y + 1][x];
                
                // 添加随机衰减
                uint8_t decay = random(0, 50);
                int16_t newVal = (int16_t)below - decay;
                if (newVal < 0) newVal = 0;
                
                // 添加左右扩散
                if (x > 0 && x < SCREEN_COLS - 1) {
                    uint8_t left = firePixels[y + 1][x - 1];
                    uint8_t right = firePixels[y + 1][x + 1];
                    newVal = (newVal * 3 + left + right) / 5;
                }
                
                firePixels[y][x] = (uint8_t)newVal;
            }
        }
    }
};

#endif // FIRE_BACKGROUND_H
