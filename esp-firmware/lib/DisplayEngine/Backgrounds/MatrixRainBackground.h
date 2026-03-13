/**
 * MatrixRainBackground - 矩阵雨效果背景
 * 
 * 修复：合并update和render，避免帧率不匹配导致的闪烁
 */

#ifndef MATRIX_RAIN_BACKGROUND_H
#define MATRIX_RAIN_BACKGROUND_H

#include "../Backgrounds/Background.h"

class MatrixRainBackground : public Background {
public:
    MatrixRainBackground() : Background(Type::MATRIX_RAIN) {
        memset(drops, 0, sizeof(drops));
        memset(speeds, 0, sizeof(speeds));
        rainLevel = 0;
        rainStateMs = 0;
        lastUpdateMs = 0;
    }
    
    void init() override {
        Background::init();
        // 初始化雨滴
        for (int x = 0; x < SCREEN_COLS; x++) {
            drops[x] = random(-20, 0);
            speeds[x] = random(40, 120) / 100.0f;
        }
        rainLevel = 0;
        rainStateMs = millis();
        lastUpdateMs = millis();
    }
    
    // 关键修复：render中直接更新，不依赖update
    void render(LEDMatrix& matrix) override {
        uint32_t now = millis();
        
        // 只有在30ms间隔才更新雨滴位置
        if (now - lastUpdateMs >= 30) {
            lastUpdateMs = now;
            
            // 切换雨量级别
            if (now - rainStateMs > 5000) {
                rainLevel = (rainLevel + 1) % 3;
                rainStateMs = now;
            }
            
            // 淡出效果
            uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
            for (int y = 0; y < SCREEN_ROWS; y++) {
                for (int x = 0; x < SCREEN_COLS; x++) {
                    uint8_t v = fb[y][x];
                    if (v > 25) fb[y][x] = v - 25;
                    else fb[y][x] = 0;
                }
            }
            
            // 更新雨滴
            int thresholds[3] = {800, 960, 995};
            int currentThreshold = thresholds[rainLevel];
            
            for (int x = 0; x < SCREEN_COLS; x++) {
                int old_iy = (int)drops[x];
                drops[x] += speeds[x];
                int iy = (int)drops[x];
                
                // 填充路径
                for (int py = old_iy; py <= iy; py++) {
                    if (py >= 0 && py < SCREEN_ROWS) {
                        fb[py][x] = 255;
                    }
                }
                
                // Respawn
                if (drops[x] > SCREEN_ROWS + 5) {
                    drops[x] = SCREEN_ROWS + 5.1f;
                    if (random(1000) > currentThreshold) {
                        drops[x] = random(-10, -1);
                        if (rainLevel == 0)
                            speeds[x] = random(80, 180) / 100.0f;
                        else if (rainLevel == 1)
                            speeds[x] = random(30, 80) / 100.0f;
                        else
                            speeds[x] = random(15, 50) / 100.0f;
                    }
                }
            }
        }
        // 如果不到30ms，不做任何操作（保持上一帧）
    }
    
    // update() 留空，所有逻辑在render中处理
    void update() override {
        // 所有更新逻辑已移到render()
    }
    
    void setRainLevel(int level) {
        if (level >= 0 && level <= 2) {
            rainLevel = level;
            rainStateMs = millis();
        }
    }
    
    int getRainLevel() const { return rainLevel; }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    float drops[SCREEN_COLS];
    float speeds[SCREEN_COLS];
    int rainLevel;
    uint32_t rainStateMs;
    uint32_t lastUpdateMs;  // 用于控制更新频率
};

#endif
