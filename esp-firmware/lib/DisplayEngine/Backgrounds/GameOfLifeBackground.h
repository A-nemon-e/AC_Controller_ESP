/**
 * GameOfLifeBackground - 生命游戏背景
 * 
 * Conway的生命游戏模拟
 * 基于 test_3733_scanner.ino 第1088-1186行 runGameOfLife() 实现
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef GAME_OF_LIFE_BACKGROUND_H
#define GAME_OF_LIFE_BACKGROUND_H

#include "../Backgrounds/Background.h"

class GameOfLifeBackground : public Background {
public:
    GameOfLifeBackground() : Background(Type::GAME_OF_LIFE) {
        updateInterval = 150;  // 150ms帧率
        memset(currentGen, 0, sizeof(currentGen));
        memset(hashHistory, 0, sizeof(hashHistory));
        historyIdx = 0;
        gameRef = 0;
        currentRef = 0;
    }
    
    void init() override {
        Background::init();
        // 初始化随机细胞 - 参照 enterGameOfLife()
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                currentGen[y][x] = (random(100) > 75) ? 255 : 0;
            }
        }
        memset(hashHistory, 0, sizeof(hashHistory));
        historyIdx = 0;
        gameRef = millis();
        if (gameRef == 0) gameRef = 1;
        currentRef = gameRef;
    }
    
    void render(LEDMatrix& matrix) override {
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        // 渲染当前代
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                fb[y][x] = currentGen[y][x];
            }
        }
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            
            // 检查是否需要重置历史（游戏引用变化）
            if (currentRef != gameRef) {
                currentRef = gameRef;
                memset(hashHistory, 0, sizeof(hashHistory));
                historyIdx = 0;
            }
            
            // 计算下一代
            calculateNextGen();
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, true, true, true, true, Position::CENTER};
    }

private:
    uint8_t currentGen[SCREEN_ROWS][SCREEN_COLS];
    uint32_t hashHistory[64];
    int historyIdx;
    uint32_t gameRef;
    uint32_t currentRef;
    
    void calculateNextGen() {
        uint8_t nextGen[SCREEN_ROWS][SCREEN_COLS];
        int aliveCount = 0;
        
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                int neighbors = 0;
                
                // 计算邻居数量（边界环绕）
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        // 边界环绕
                        if (nx < 0) nx = SCREEN_COLS - 1;
                        if (nx >= SCREEN_COLS) nx = 0;
                        if (ny < 0) ny = SCREEN_ROWS - 1;
                        if (ny >= SCREEN_ROWS) ny = 0;
                        
                        if (currentGen[ny][nx] > 0) neighbors++;
                    }
                }
                
                // Conway规则
                if (currentGen[y][x] > 0) {
                    if (neighbors < 2 || neighbors > 3)
                        nextGen[y][x] = 0;
                    else {
                        nextGen[y][x] = 255;
                        aliveCount++;
                    }
                } else {
                    if (neighbors == 3) {
                        nextGen[y][x] = 255;
                        aliveCount++;
                    } else {
                        nextGen[y][x] = 0;
                    }
                }
            }
        }
        
        // 计算FNV-1a哈希
        uint32_t currentHash = 2166136261u;
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                currentGen[y][x] = nextGen[y][x];
                currentHash ^= currentGen[y][x];
                currentHash *= 16777619u;
            }
        }
        
        // 检测循环
        bool isLoop = false;
        for (int i = 0; i < 64; i++) {
            if (hashHistory[i] != 0 && hashHistory[i] == currentHash) {
                isLoop = true;
                break;
            }
        }
        
        hashHistory[historyIdx] = currentHash;
        historyIdx = (historyIdx + 1) % 64;
        
        // 如果陷入循环或全部死亡，重新初始化
        if (isLoop || aliveCount == 0) {
            // 延迟后重新初始化
            if (aliveCount == 0) {
                // 立即重新初始化
                for (int y = 0; y < SCREEN_ROWS; y++) {
                    for (int x = 0; x < SCREEN_COLS; x++) {
                        currentGen[y][x] = (random(100) > 75) ? 255 : 0;
                    }
                }
            } else {
                // 循环检测 - 也重新初始化
                for (int y = 0; y < SCREEN_ROWS; y++) {
                    for (int x = 0; x < SCREEN_COLS; x++) {
                        currentGen[y][x] = (random(100) > 75) ? 255 : 0;
                    }
                }
            }
            memset(hashHistory, 0, sizeof(hashHistory));
            historyIdx = 0;
        }
    }
};

#endif // GAME_OF_LIFE_BACKGROUND_H
