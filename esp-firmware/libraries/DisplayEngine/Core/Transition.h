/**
 * Transition - 转场效果系统
 * 
 * 实现卡片切换时的过渡动画效果
 * 
 * @author AI Assistant
 * @version 1.0.0
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

class Transition {
public:
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
    
    // 获取当前进度 0.0-1.0
    float getProgress() const {
        if (!isActive) return 1.0f;
        uint32_t elapsed = millis() - startTime;
        if (elapsed >= durationMs) return 1.0f;
        return (float)elapsed / (float)durationMs;
    }
    
    // 混合两个Buffer到目标Buffer
    void blend(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
               const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
               uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS]) {
        
        float progress = getProgress();
        
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
    
    void blendFade(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                   const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                   float progress) {
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint8_t fromVal = fromBuffer[y][x];
                uint8_t toVal = toBuffer[y][x];
                outputBuffer[y][x] = (uint8_t)(fromVal * (1.0f - progress) + toVal * progress);
            }
        }
    }
    
    void blendSlide(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                    const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                    uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                    float progress, bool horizontal, bool reverse) {
        
        // 清空输出
        memset(outputBuffer, 0, SCREEN_ROWS * SCREEN_COLS);
        
        if (horizontal) {
            int offset = (int)(SCREEN_COLS * progress);
            if (reverse) offset = -offset;
            
            for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                    // From buffer sliding out
                    int fromX = reverse ? (int)x - offset : (int)x + offset;
                    if (fromX >= 0 && fromX < SCREEN_COLS) {
                        outputBuffer[y][x] = fromBuffer[y][(uint8_t)fromX];
                    }
                    
                    // To buffer sliding in
                    int toX = reverse ? (int)x + SCREEN_COLS - offset : (int)x - SCREEN_COLS + offset;
                    if (toX >= 0 && toX < SCREEN_COLS) {
                        outputBuffer[y][x] = toBuffer[y][(uint8_t)toX];
                    }
                }
            }
        } else {
            int offset = (int)(SCREEN_ROWS * progress);
            if (reverse) offset = -offset;
            
            for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
                for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                    // From buffer sliding out
                    int fromY = reverse ? (int)y - offset : (int)y + offset;
                    if (fromY >= 0 && fromY < SCREEN_ROWS) {
                        outputBuffer[y][x] = fromBuffer[(uint8_t)fromY][x];
                    }
                    
                    // To buffer sliding in
                    int toY = reverse ? (int)y + SCREEN_ROWS - offset : (int)y - SCREEN_ROWS + offset;
                    if (toY >= 0 && toY < SCREEN_ROWS) {
                        outputBuffer[y][x] = toBuffer[(uint8_t)toY][x];
                    }
                }
            }
        }
    }
    
    void blendWipe(const uint8_t fromBuffer[SCREEN_ROWS][SCREEN_COLS],
                   const uint8_t toBuffer[SCREEN_ROWS][SCREEN_COLS],
                   uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS],
                   float progress) {
        uint8_t thresholdX = (uint8_t)(SCREEN_COLS * progress);
        
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
