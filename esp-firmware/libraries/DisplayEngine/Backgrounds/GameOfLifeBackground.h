/**
 * GameOfLifeBackground - 生命游戏背景
 * 
 * Conway的生命游戏模拟
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef GAME_OF_LIFE_BACKGROUND_H
#define GAME_OF_LIFE_BACKGROUND_H

#include "../Backgrounds/Background.h"

class GameOfLifeBackground : public Background {
public:
    GameOfLifeBackground() : Background(Type::GAME_OF_LIFE) {
        updateInterval = 200;  // 5fps
        memset(currentGen, 0, sizeof(currentGen));
        memset(nextGen, 0, sizeof(nextGen));
        generation = 0;
        stableCount = 0;
    }
    
    void init() override {
        Background::init();
        randomSeed(millis());
        initRandom();
        generation = 0;
        stableCount = 0;
    }
    
    void render(LEDMatrix& matrix) override {
        // 渲染当前代
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (currentGen[y][x]) {
                    matrix.setPixel(x, y, brightness);
                } else {
                    matrix.setPixel(x, y, 0);
                }
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            // 计算下一代
            calculateNextGen();
            
            // 检查是否稳定
            if (isStable()) {
                stableCount++;
                if (stableCount > 5) {
                    // 重新随机初始化
                    initRandom();
                    stableCount = 0;
                }
            } else {
                stableCount = 0;
            }
            
            // 交换缓冲区
            bool (*temp)[SCREEN_COLS] = currentGen;
            currentGen = nextGen;
            nextGen = temp;
            
            generation++;
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }
    
    uint32_t getGeneration() const { return generation; }

private:
    bool buffer1[SCREEN_ROWS][SCREEN_COLS];
    bool buffer2[SCREEN_ROWS][SCREEN_COLS];
    bool (*currentGen)[SCREEN_COLS];
    bool (*nextGen)[SCREEN_COLS];
    uint32_t generation;
    uint8_t stableCount;
    
    void initRandom() {
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                currentGen[y][x] = random(100) < 30;  // 30%概率存活
            }
        }
    }
    
    void calculateNextGen() {
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint8_t neighbors = countNeighbors(x, y);
                
                // Conway规则
                if (currentGen[y][x]) {
                    // 存活: 2或3个邻居存活
                    nextGen[y][x] = (neighbors == 2 || neighbors == 3);
                } else {
                    // 诞生: 3个邻居存活
                    nextGen[y][x] = (neighbors == 3);
                }
            }
        }
    }
    
    uint8_t countNeighbors(uint8_t x, uint8_t y) {
        uint8_t count = 0;
        for (int8_t dy = -1; dy <= 1; dy++) {
            for (int8_t dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                
                int16_t nx = x + dx;
                int16_t ny = y + dy;
                
                // 边界环绕
                if (nx < 0) nx = SCREEN_COLS - 1;
                if (nx >= SCREEN_COLS) nx = 0;
                if (ny < 0) ny = SCREEN_ROWS - 1;
                if (ny >= SCREEN_ROWS) ny = 0;
                
                if (currentGen[ny][nx]) count++;
            }
        }
        return count;
    }
    
    bool isStable() {
        // 检查是否和上一代相同
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (currentGen[y][x] != nextGen[y][x]) {
                    return false;
                }
            }
        }
        return true;
    }
};

#endif // GAME_OF_LIFE_BACKGROUND_H
