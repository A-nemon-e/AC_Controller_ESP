/**
 * SandBackground - 沙漏模拟背景
 * 
 * 模拟沙漏中沙子流动效果
 * 基于 test_3733_scanner.ino 第1542-1618行 runSand() 实现
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef SAND_BACKGROUND_H
#define SAND_BACKGROUND_H

#include "../Backgrounds/Background.h"

#define SAND_MAX (SCREEN_ROWS * SCREEN_COLS)

class SandBackground : public Background {
public:
    SandBackground() : Background(Type::SAND) {
        updateInterval = 45;  // 45ms帧率
        memset(sandGrid, 0, sizeof(sandGrid));
        sandCount = 0;
        lastSpawn = 0;
        lastSx = -1;
    }
    
    void init() override {
        Background::init();
        memset(sandGrid, 0, sizeof(sandGrid));
        sandCount = 0;
        lastSpawn = 0;
        lastSx = -1;
    }
    
    void render(LEDMatrix& matrix) override {
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        // 渲染沙子
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                fb[y][x] = (uint8_t)(sandGrid[y][x] > 255 ? 255 : sandGrid[y][x]);
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            // 生成沙子（60ms间隔）
            if (now - lastSpawn > 60 && sandCount < SAND_MAX - 2) {
                lastSpawn = now;
                
                // 15%概率重用上一列
                int sx;
                if (lastSx >= 0 && random(100) < 15) {
                    sx = lastSx;
                } else {
                    sx = random(0, SCREEN_COLS);
                }
                
                if (sandGrid[0][sx] == 0) {
                    sandGrid[0][sx] = random(160, 255);
                    sandCount++;
                    lastSx = sx;
                } else {
                    lastSx = -1;
                }
            }
            
            // 模拟：从底部向上处理
            for (int y = SCREEN_ROWS - 2; y >= 0; y--) {
                for (int x = 0; x < SCREEN_COLS; x++) {
                    if (sandGrid[y][x] == 0) continue;
                    
                    uint16_t val = sandGrid[y][x];
                    
                    // 尝试正下方
                    if (sandGrid[y + 1][x] == 0) {
                        sandGrid[y + 1][x] = val;
                        sandGrid[y][x] = 0;
                    } else {
                        // 尝试对角线滑动
                        int dir = (random(2) == 0) ? 1 : -1;
                        int nx = x + dir;
                        if (nx >= 0 && nx < SCREEN_COLS && sandGrid[y + 1][nx] == 0) {
                            sandGrid[y + 1][nx] = val;
                            sandGrid[y][x] = 0;
                        } else {
                            nx = x - dir;
                            if (nx >= 0 && nx < SCREEN_COLS && sandGrid[y + 1][nx] == 0) {
                                sandGrid[y + 1][nx] = val;
                                sandGrid[y][x] = 0;
                            }
                            // 否则卡住不动
                        }
                    }
                }
            }
            
            // 屏幕满后自动清空
            if (sandCount >= SAND_MAX - 10) {
                memset(sandGrid, 0, sizeof(sandGrid));
                sandCount = 0;
            }
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    uint16_t sandGrid[SCREEN_ROWS][SCREEN_COLS];
    int sandCount;
    uint32_t lastSpawn;
    int lastSx;
};

#endif // SAND_BACKGROUND_H
