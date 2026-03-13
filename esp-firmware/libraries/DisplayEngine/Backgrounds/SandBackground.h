/**
 * SandBackground - 沙漏模拟背景
 * 
 * 模拟沙漏中沙子流动效果
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef SAND_BACKGROUND_H
#define SAND_BACKGROUND_H

#include "../Backgrounds/Background.h"

class SandBackground : public Background {
public:
    SandBackground() : Background(Type::SAND) {
        updateInterval = 30;  // ~33fps
        memset(grid, 0, sizeof(grid));
        memset(nextGrid, 0, sizeof(nextGrid));
        sandCount = 0;
        totalSand = SCREEN_COLS * SCREEN_ROWS / 3;  // 1/3屏幕填充
    }
    
    void init() override {
        Background::init();
        memset(grid, 0, sizeof(grid));
        memset(nextGrid, 0, sizeof(nextGrid));
        sandCount = 0;
        
        // 在顶部生成沙子
        for (uint8_t y = 0; y < 3 && sandCount < totalSand; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS && sandCount < totalSand; x++) {
                if (random(100) < 70) {
                    grid[y][x] = 1;
                    sandCount++;
                }
            }
        }
    }
    
    void render(LEDMatrix& matrix) override {
        // 渲染沙子
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (grid[y][x]) {
                    matrix.setPixel(x, y, brightness);
                } else {
                    matrix.setPixel(x, y, 0);
                }
            }
        }
        
        // 更新沙子位置
        updateSand();
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            updateSand();
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    uint8_t grid[SCREEN_ROWS][SCREEN_COLS];
    uint8_t nextGrid[SCREEN_ROWS][SCREEN_COLS];
    uint16_t sandCount;
    uint16_t totalSand;
    
    void updateSand() {
        // 复制当前状态
        memcpy(nextGrid, grid, sizeof(grid));
        
        // 从底部向上处理（让沙子下落）
        for (int8_t y = SCREEN_ROWS - 2; y >= 0; y--) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (grid[y][x]) {
                    // 尝试下落
                    if (!grid[y+1][x]) {
                        // 正下方为空
                        nextGrid[y][x] = 0;
                        nextGrid[y+1][x] = 1;
                    } else if (x > 0 && !grid[y+1][x-1]) {
                        // 左下为空
                        nextGrid[y][x] = 0;
                        nextGrid[y+1][x-1] = 1;
                    } else if (x < SCREEN_COLS - 1 && !grid[y+1][x+1]) {
                        // 右下为空
                        nextGrid[y][x] = 0;
                        nextGrid[y+1][x+1] = 1;
                    }
                }
            }
        }
        
        // 交换缓冲区
        memcpy(grid, nextGrid, sizeof(grid));
        
        // 补充顶部沙子
        replenishSand();
    }
    
    void replenishSand() {
        uint16_t currentSand = 0;
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (grid[y][x]) currentSand++;
            }
        }
        
        // 如果沙子太少，补充到顶部
        if (currentSand < totalSand * 0.8) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (!grid[0][x] && random(100) < 10) {
                    grid[0][x] = 1;
                }
            }
        }
    }
};

#endif // SAND_BACKGROUND_H
