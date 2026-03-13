/**
 * MatrixRainBackground - 矩阵雨效果背景
 * 
 * 经典黑客帝国风格数字雨
 * 基于 test_3733_scanner.ino 第830-929行 runMatrixRain() 实现
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef MATRIX_RAIN_BACKGROUND_H
#define MATRIX_RAIN_BACKGROUND_H

#include "../Backgrounds/Background.h"

class MatrixRainBackground : public Background {
public:
    MatrixRainBackground() : Background(Type::MATRIX_RAIN) {
        updateInterval = 30;  // 30ms帧率，与test_3733_scanner一致
        memset(drops, 0, sizeof(drops));
        memset(speeds, 0, sizeof(speeds));
        rainLevel = 0;  // 0: Heavy, 1: Medium, 2: Light
        rainStateMs = 0;
    }
    
    void init() override {
        Background::init();
        // 初始化雨滴 - 参照 enterMatrixRain()
        for (int x = 0; x < SCREEN_COLS; x++) {
            drops[x] = random(-20, 0);
            speeds[x] = random(40, 120) / 100.0f;
        }
        rainLevel = 0;
        rainStateMs = millis();
    }
    
    void render(LEDMatrix& matrix) override {
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        // 1. 淡出效果 (fade = 25)
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                uint8_t v = fb[y][x];
                int fade = 25;
                if (v > fade)
                    fb[y][x] = v - fade;
                else
                    fb[y][x] = 0;
            }
        }
        
        // 2. 更新雨滴位置并绘制
        int thresholds[3] = {800, 960, 995};  // Heavy, Medium, Light
        int currentThreshold = thresholds[rainLevel];
        
        for (int x = 0; x < SCREEN_COLS; x++) {
            int old_iy = (int)drops[x];
            drops[x] += speeds[x];
            int iy = (int)drops[x];
            
            // 填充路径（防止高速雨滴断裂）
            for (int py = old_iy; py <= iy; py++) {
                if (py >= 0 && py < SCREEN_ROWS) {
                    fb[py][x] = 255;
                }
            }
            
            // Respawn logic
            if (drops[x] > SCREEN_ROWS + 5) {
                drops[x] = SCREEN_ROWS + 5.1f;
                if (random(1000) > currentThreshold) {
                    drops[x] = random(-10, -1);
                    if (rainLevel == 0)
                        speeds[x] = random(80, 180) / 100.0f;  // Heavy: very fast
                    else if (rainLevel == 1)
                        speeds[x] = random(30, 80) / 100.0f;   // Medium: normal
                    else
                        speeds[x] = random(15, 50) / 100.0f;   // Light: slow
                }
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            // 每5秒切换雨量级别
            if (now - rainStateMs > 5000) {
                rainLevel = (rainLevel + 1) % 3;
                rainStateMs = now;
            }
        }
    }
    
    void setRainLevel(int level) {
        if (level >= 0 && level <= 2) {
            rainLevel = level;
            rainStateMs = millis(); // 重置计时器
        }
    }
    
    int getRainLevel() const {
        return rainLevel;
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    float drops[SCREEN_COLS];      // 雨滴位置（float类型）
    float speeds[SCREEN_COLS];     // 雨滴速度
    int rainLevel;                 // 雨量级别 0=Heavy, 1=Medium, 2=Light
    uint32_t rainStateMs;          // 雨量状态切换时间
};

#endif // MATRIX_RAIN_BACKGROUND_H
