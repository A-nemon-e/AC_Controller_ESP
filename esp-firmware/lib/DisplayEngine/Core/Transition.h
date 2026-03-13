/**
 * Transition - 转场效果系统 (定点数优化版)
 * 
 * 实现卡片切换时的过渡动画效果
 * 优化：使用定点数(0-255)替代浮点数运算，提高ESP8266性能
 * 
 * @author AI Assistant
 * @version 1.1.0
 */

#ifndef TRANSITION_H
#define TRANSITION_H

#include <Arduino.h>
#include "../LEDMatrix/LEDMatrix.h"

enum class TransitionType {
    INSTANT,        // 硬切（无效果）
    FADE,           // 淡入淡出
    SLIDE_LEFT,     // 向左滑动
    SLIDE_RIGHT,    // 向右滑动
    SLIDE_UP,       // 向上滑动
    SLIDE_DOWN,     // 向下滑动
    WIPE            // 擦除效果
};

/**
 * 转场效果类 - 使用定点数优化
 * 进度使用0-255的整数表示，避免浮点运算
 */
class Transition {
public:
    static constexpr uint8_t PROGRESS_MAX = 255;  // 最大进度值(定点数1.0)
    
    Transition(TransitionType type = TransitionType::FADE, uint16_t durationMs = 500)
        : type(type), durationMs(durationMs), startTime(0), isActive(false) {}
    
    void setType(TransitionType t) { type = t; }
    TransitionType getType() const { return type; }
    void setDuration(uint16_t ms) { durationMs = ms; }
    
    void begin() {
        startTime = millis();
        isActive = true;
    }
    
    void update() {
        if (isActive && isFinished()) {
            isActive = false;
        }
    }
    
    bool isFinished() const {
        if (!isActive) return true;
        return (millis() - startTime) >= durationMs;
    }
    
    bool isInProgress() const {
        return isActive && !isFinished();
    }
    
    /**
     * 获取当前进度 0-255 (定点数优化)
     * 替代原来的0.0-1.0 float
     */
    uint8_t getProgressFixed() const {
        if (!isActive) return PROGRESS_MAX;
        uint32_t elapsed = millis() - startTime;
        if (elapsed >= durationMs) return PROGRESS_MAX;
        // 整数除法: (elapsed * 255) / durationMs
        return (uint8_t)((elapsed * (uint32_t)PROGRESS_MAX) / durationMs);
    }
    
    /**
     * 获取当前进度 0.0-1.0 (仅用于兼容性，内部不再使用)
     */
    float getProgress() const {
        return (float)getProgressFixed() / PROGRESS_MAX;
    }
    
    // 混合两个Buffer到目标Buffer
    void blend(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
               const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
               uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS]) {
        
        uint8_t progress = getProgressFixed();
        
        switch (type) {
            case TransitionType::INSTANT:
                blendInstant(toBuffer, outputBuffer);
                break;
            case TransitionType::FADE:
                blendFade(fromBuffer, toBuffer, outputBuffer, progress);
                break;
            case TransitionType::SLIDE_LEFT:
                blendSlide(fromBuffer, toBuffer, outputBuffer, progress, true, true);
                break;
            case TransitionType::SLIDE_RIGHT:
                blendSlide(fromBuffer, toBuffer, outputBuffer, progress, true, false);
                break;
            case TransitionType::SLIDE_UP:
                blendSlide(fromBuffer, toBuffer, outputBuffer, progress, false, true);
                break;
            case TransitionType::SLIDE_DOWN:
                blendSlide(fromBuffer, toBuffer, outputBuffer, progress, false, false);
                break;
            case TransitionType::WIPE:
                blendWipe(fromBuffer, toBuffer, outputBuffer, progress);
                break;
        }
    }

private:
    TransitionType type;
    uint16_t durationMs;
    uint32_t startTime;
    bool isActive;
    
    void blendInstant(const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                      uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS]) {
        memcpy(outputBuffer, toBuffer, SCREEN_ROWS * SCREEN_COLS);
    }
    
    /**
     * 淡入淡出混合 - 使用定点数优化
     * progress: 0-255，表示0.0-1.0
     * formula: result = from * (255 - progress) / 255 + to * progress / 255
     * simplified: result = (from * (255 - progress) + to * progress) / 255
     */
    void blendFade(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                   const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t progress) {
        // 预计算反向进度
        uint8_t invProgress = PROGRESS_MAX - progress;
        
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint16_t fromVal = fromBuffer[y][x];
                uint16_t toVal = toBuffer[y][x];
                // 定点数线性插值: (from * inv + to * progress) / 255
                uint16_t result = (fromVal * invProgress + toVal * progress) >> 8;
                outputBuffer[y][x] = (uint8_t)result;
            }
        }
    }
    
    /**
     * 滑动混合 - 使用定点数优化
     * progress: 0-255，表示0.0-1.0
     */
    void blendSlide(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                    const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                    uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                    uint8_t progress, bool horizontal, bool reverse) {
        
        // 清空输出
        memset(outputBuffer, 0, SCREEN_ROWS * SCREEN_COLS);
        
        if (horizontal) {
            // 使用定点数计算偏移量: offset = cols * progress / 255
            uint8_t offset = (uint8_t)(((uint16_t)SCREEN_COLS * progress) >> 8);
            if (reverse) {
                // 向左滑: from向右移，to从左边进入
                for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                    for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                        // from buffer: 向右移动
                        if (x >= offset) {
                            uint8_t fromX = x - offset;
                            outputBuffer[y][x] = fromBuffer[y][fromX];
                        }
                        // to buffer: 从左边进入
                        uint8_t toX = x + SCREEN_COLS - offset;
                        if (toX < SCREEN_COLS) {
                            outputBuffer[y][x] = toBuffer[y][toX];
                        }
                    }
                }
            } else {
                // 向右滑: from向左移，to从右边进入
                for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                    for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                        // from buffer: 向左移动
                        if (x + offset < SCREEN_COLS) {
                            uint8_t fromX = x + offset;
                            outputBuffer[y][x] = fromBuffer[y][fromX];
                        }
                        // to buffer: 从右边进入
                        if (x >= offset) {
                            uint8_t toX = x - offset;
                            outputBuffer[y][x] = toBuffer[y][toX];
                        }
                    }
                }
            }
        } else {
            // 使用定点数计算偏移量: offset = rows * progress / 255
            uint8_t offset = (uint8_t)(((uint16_t)SCREEN_ROWS * progress) >> 8);
            if (reverse) {
                // 向上滑: from向下移，to从上边进入
                for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                    for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                        // from buffer: 向下移动
                        if (y >= offset) {
                            uint8_t fromY = y - offset;
                            outputBuffer[y][x] = fromBuffer[fromY][x];
                        }
                        // to buffer: 从上边进入
                        uint8_t toY = y + SCREEN_ROWS - offset;
                        if (toY < SCREEN_ROWS) {
                            outputBuffer[y][x] = toBuffer[toY][x];
                        }
                    }
                }
            } else {
                // 向下滑: from向上移，to从下边进入
                for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                    for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                        // from buffer: 向上移动
                        if (y + offset < SCREEN_ROWS) {
                            uint8_t fromY = y + offset;
                            outputBuffer[y][x] = fromBuffer[fromY][x];
                        }
                        // to buffer: 从下边进入
                        if (y >= offset) {
                            uint8_t toY = y - offset;
                            outputBuffer[y][x] = toBuffer[toY][x];
                        }
                    }
                }
            }
        }
    }
    
    /**
     * 擦除效果 - 使用定点数优化
     * progress: 0-255，表示0.0-1.0
     */
    void blendWipe(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                   const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t progress) {
        // 定点数计算阈值: threshold = cols * progress / 255
        uint8_t thresholdX = (uint8_t)(((uint16_t)SCREEN_COLS * progress) >> 8);
        
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                if (x < thresholdX) {
                    outputBuffer[y][x] = toBuffer[y][x];
                } else {
                    outputBuffer[y][x] = fromBuffer[y][x];
                }
            }
        }
    }
};

#endif // TRANSITION_H
