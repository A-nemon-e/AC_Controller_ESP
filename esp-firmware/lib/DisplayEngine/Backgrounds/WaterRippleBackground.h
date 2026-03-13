/**
 * WaterRippleBackground - 水波纹效果背景
 * 
 * 模拟水波纹扩散效果
 * 基于 test_3733_scanner.ino 第1015-1038行 runWaterRipples() 实现
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef WATER_RIPPLE_BACKGROUND_H
#define WATER_RIPPLE_BACKGROUND_H

#include "../Backgrounds/Background.h"
#include <math.h>

class WaterRippleBackground : public Background {
public:
    WaterRippleBackground() : Background(Type::WATER_RIPPLE) {
        updateInterval = 20;  // 20ms帧率，与smartDelay(20)一致
        t = 0.0f;
    }
    
    void init() override {
        Background::init();
        t = 0.0f;
    }
    
    void render(LEDMatrix& matrix) override {
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        float cx = SCREEN_COLS / 2.0f - 0.5f;
        float cy = SCREEN_ROWS / 2.0f - 0.5f;
        
        // 更新时间
        t += 0.15f;
        if (t > M_PI * 2) {
            t -= M_PI * 2;
        }
        
        // 生成水波纹
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float dist = sqrtf(dx * dx + dy * dy);
                float val = sinf(dist * 0.8f - t);
                uint8_t bri = (uint8_t)((val + 1.0f) * 127.5f);
                fb[y][x] = bri;
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    float t;  // 时间变量
};

#endif // WATER_RIPPLE_BACKGROUND_H
