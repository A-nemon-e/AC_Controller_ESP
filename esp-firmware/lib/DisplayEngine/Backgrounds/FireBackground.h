/**
 * FireBackground - 火焰效果背景
 * 
 * 基于DOOM经典火焰算法的动态火焰动画
 * 基于 test_3733_scanner.ino 第1189-1322行 runFire() 实现
 * 包含信息覆盖层（时间/日期/温湿度）
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef FIRE_BACKGROUND_H
#define FIRE_BACKGROUND_H

#include "../Backgrounds/Background.h"

class FireBackground : public Background {
public:
    FireBackground() : Background(Type::FIRE) {
        updateInterval = 45;  // 45ms帧率，与test_3733_scanner一致
        memset(firePixels, 0, sizeof(firePixels));
        stateTimer = 0;
        displayState = 0;  // 0=Time(8s), 1=Date(3s), 2=AHT(2.5s)
    }
    
    void init() override {
        Background::init();
        memset(firePixels, 0, sizeof(firePixels));
        stateTimer = millis();
        displayState = 0;
    }
    
    void render(LEDMatrix& matrix) override {
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        // 1. 在底部隐藏行生成燃料
        for (int x = 0; x < SCREEN_COLS; x++) {
            if (random(100) > 20) {
                firePixels[x][SCREEN_ROWS + 1] = random(150, 255);
            } else {
                firePixels[x][SCREEN_ROWS + 1] = random(20, 80);
            }
        }
        
        // 2. DOOM火焰传播算法
        for (int x = 0; x < SCREEN_COLS; x++) {
            for (int y = 0; y <= SCREEN_ROWS; y++) {
                int srcIntensity = firePixels[x][y + 1];
                
                // 随机衰减
                int decay = random(7, 67);
                int newIntensity = srcIntensity - decay;
                if (newIntensity < 0) newIntensity = 0;
                
                // 随机漂移
                int drift = random(0, 3) - 1;  // -1, 0, 1
                int targetX = x + drift;
                
                // 边界环绕
                if (targetX < 0) targetX = SCREEN_COLS - 1;
                else if (targetX >= SCREEN_COLS) targetX = 0;
                
                firePixels[targetX][y] = newIntensity;
            }
        }
        
        // 3. 水平平滑并渲染到屏幕
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                int left = (x > 0) ? firePixels[x - 1][y] : firePixels[SCREEN_COLS - 1][y];
                int right = (x < SCREEN_COLS - 1) ? firePixels[x + 1][y] : firePixels[0][y];
                int center = firePixels[x][y];
                
                int smoothed = (center * 2 + left + right) / 4;
                fb[y][x] = (uint8_t)smoothed;
            }
        }
        
        // 4. 信息覆盖层（简化版，实际应通过时间服务获取）
        uint32_t nowMs = millis();
        uint32_t duration = (displayState == 0) ? 8000 : (displayState == 1 ? 3000 : 2500);
        
        if (nowMs - stateTimer > duration) {
            stateTimer = nowMs;
            displayState = (displayState + 1) % 3;
        }
        
        // 在火焰上绘制文字（简化实现，亮度0xFF）
        // 注意：实际实现需要字体渲染支持
        // 这里预留接口，由DisplayManager处理文字叠加
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
        return {true, false, false, false, false, Position::TOP_CENTER};
    }

private:
    // 火焰像素数组，底部有2行隐藏行用于生成燃料
    uint8_t firePixels[SCREEN_COLS][SCREEN_ROWS + 2];
    uint32_t stateTimer;
    int displayState;  // 0=Time, 1=Date, 2=AHT
};

#endif // FIRE_BACKGROUND_H
