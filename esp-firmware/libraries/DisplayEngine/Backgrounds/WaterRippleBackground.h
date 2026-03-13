/**
 * WaterRippleBackground - 水波纹效果背景
 * 
 * 模拟水波纹扩散效果
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef WATER_RIPPLE_BACKGROUND_H
#define WATER_RIPPLE_BACKGROUND_H

#include "../Backgrounds/Background.h"

class WaterRippleBackground : public Background {
public:
    WaterRippleBackground() : Background(Type::WATER_RIPPLE) {
        updateInterval = 40;  // 25fps
        memset(buffer1, 0, sizeof(buffer1));
        memset(buffer2, 0, sizeof(buffer2));
        currentBuffer = buffer1;
        prevBuffer = buffer2;
        damping = 8;  // 衰减因子
    }
    
    void init() override {
        Background::init();
        memset(buffer1, 0, sizeof(buffer1));
        memset(buffer2, 0, sizeof(buffer2));
        currentBuffer = buffer1;
        prevBuffer = buffer2;
    }
    
    void render(LEDMatrix& matrix) override {
        // 渲染当前状态
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                int16_t val = currentBuffer[y][x];
                // 将值映射到亮度
                if (val < 0) val = -val;
                if (val > 255) val = 255;
                matrix.setPixel(x, y, (uint16_t)val * brightness / 255);
            }
        }
        
        // 更新波纹
        updateRipple();
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            // 随机添加新波纹
            if (random(100) < 10) {  // 10%概率
                addRipple(random(SCREEN_COLS), random(SCREEN_ROWS), random(200, 500));
            }
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }
    
    // 在指定位置添加波纹
    void addRipple(uint8_t x, uint8_t y, int16_t strength) {
        if (x > 0 && x < SCREEN_COLS - 1 && y > 0 && y < SCREEN_ROWS - 1) {
            currentBuffer[y][x] = strength;
        }
    }

private:
    int16_t buffer1[SCREEN_ROWS][SCREEN_COLS];
    int16_t buffer2[SCREEN_ROWS][SCREEN_COLS];
    int16_t (*currentBuffer)[SCREEN_COLS];
    int16_t (*prevBuffer)[SCREEN_COLS];
    uint8_t damping;
    
    void updateRipple() {
        // 交换缓冲区
        int16_t (*temp)[SCREEN_COLS] = currentBuffer;
        currentBuffer = prevBuffer;
        prevBuffer = temp;
        
        // 计算新状态
        for (uint8_t y = 1; y < SCREEN_ROWS - 1; y++) {
            for (uint8_t x = 1; x < SCREEN_COLS - 1; x++) {
                // 波纹公式: 新值 = (左+右+上+下)/2 - 上一帧的值
                int16_t val = (prevBuffer[y][x-1] + 
                               prevBuffer[y][x+1] + 
                               prevBuffer[y-1][x] + 
                               prevBuffer[y+1][x]) / 2 - currentBuffer[y][x];
                
                // 衰减
                val -= val / damping;
                
                // 限制范围
                if (val > 1000) val = 1000;
                if (val < -1000) val = -1000;
                
                currentBuffer[y][x] = val;
            }
        }
    }
};

#endif // WATER_RIPPLE_BACKGROUND_H
