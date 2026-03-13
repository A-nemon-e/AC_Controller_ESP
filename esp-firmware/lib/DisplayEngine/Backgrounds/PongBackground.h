/**
 * PongBackground - Pong游戏时钟背景
 * 
 * 经典Pong游戏作为时钟显示
 * 基于 test_3733_scanner.ino 第1324-1499行 runPongClock() 实现
 * 
 * @author AI Assistant
 * @version 2.0.0
 */

#ifndef PONG_BACKGROUND_H
#define PONG_BACKGROUND_H

#include "../Backgrounds/Background.h"
#include "../FontRenderer/FontRenderer.h"
#include <math.h>

class PongBackground : public Background {
public:
    PongBackground() : Background(Type::PONG) {
        updateInterval = 35;  // 35ms帧率
        leftY = 5.0f;
        rightY = 5.0f;
        ballX = 21.0f;
        ballY = 5.0f;
        ballVx = 0.25f;
        ballVy = 0.15f;
        leftScore = 0;
        rightScore = 0;
        baseMs = 0;
        lastSec = -1;
    }
    
    void init() override {
        Background::init();
        leftY = 5.0f;
        rightY = 5.0f;
        ballX = 21.0f;
        ballY = 5.0f;
        ballVx = 0.25f;
        ballVy = 0.15f;
        leftScore = 0;
        rightScore = 0;
        baseMs = 0;
        lastSec = -1;
    }
    
    void render(LEDMatrix& matrix) override {
        uint32_t now = millis();
        
        // 只有在35ms间隔才更新
        if (now - lastUpdateMs < 35) {
            return;
        }
        lastUpdateMs = now;
        
        // 获取缓冲区直接操作
        uint8_t (*fb)[SCREEN_COLS] = LEDMatrix::getBuffer();
        
        // 清空缓冲区
        for (int y = 0; y < SCREEN_ROWS; y++) {
            for (int x = 0; x < SCREEN_COLS; x++) {
                fb[y][x] = 0;
            }
        }
        
        // 获取当前时间（简化版，实际应通过时间服务获取）
        // 这里使用millis模拟秒数
        uint32_t nowMs = millis();
        int curSec = (nowMs / 1000) % 60;
        int curMin = (nowMs / 60000) % 60;
        int curHour = ((nowMs / 3600000) % 24);
        
        // 同步毫秒时间线
        if (curSec != lastSec) {
            uint32_t expectedMs = curSec * 1000;
            uint32_t currentSimMs = nowMs - baseMs;
            if (lastSec == -1 || curSec < lastSec || 
                abs((int)currentSimMs - (int)expectedMs) > 1500) {
                baseMs = nowMs - expectedMs;
            }
            lastSec = curSec;
        }
        
        uint32_t totalMs = (nowMs - baseMs) % 60000;
        float totalSec = totalMs / 1000.0f;
        
        // 计算球位置
        bool movingLeft;
        float progress;
        
        if (totalSec <= 55.0f) {
            // Phase 1: 0-55秒正常对打
            float phasePos = fmod(totalSec, 11.0f);
            if (phasePos < 5.5f) {
                movingLeft = true;
                progress = phasePos / 5.5f;
            } else {
                movingLeft = false;
                progress = (phasePos - 5.5f) / 5.5f;
            }
        } else {
            // Phase 2: 55-59秒慢速穿越
            movingLeft = true;
            progress = (totalSec - 55.0f) / 4.0f;
        }
        
        if (movingLeft) {
            ballX = 39.0f - (37.0f * progress);
        } else {
            ballX = 2.0f + (37.0f * progress);
        }
        
        // 59秒后飞出
        if (totalSec >= 59.0f) {
            ballX = 2.0f - (37.0f * ((totalSec - 59.0f) / 1.0f));
        }
        
        // 计算球Y（简单弹跳）
        float bounceY = fmod(totalSec * 3.5f, (SCREEN_ROWS - 2) * 2);
        if (bounceY > SCREEN_ROWS - 2) {
            ballY = (SCREEN_ROWS - 2) - (bounceY - (SCREEN_ROWS - 2));
        } else {
            ballY = 0.5f + bounceY;
        }
        
        // 更新比分
        leftScore = curHour;
        rightScore = curMin;
        
        // AI移动
        float targetL = ballY;
        if (totalSec >= 58.0f) {
            targetL = (ballY <= 5.0f) ? SCREEN_ROWS - 1.5f : 1.5f;
        }
        float diffL = targetL - leftY;
        if (fabsf(diffL) > 0.5f)
            leftY += (diffL > 0 ? 0.25f : -0.25f);
        
        float targetR = ballY;
        float diffR = targetR - rightY;
        if (fabsf(diffR) > 0.5f)
            rightY += (diffR > 0 ? 0.25f : -0.25f);
        
        // 限制球拍位置
        if (leftY < 1.5f) leftY = 1.5f;
        if (leftY > SCREEN_ROWS - 2.5f) leftY = SCREEN_ROWS - 2.5f;
        if (rightY < 1.5f) rightY = 1.5f;
        if (rightY > SCREEN_ROWS - 2.5f) rightY = SCREEN_ROWS - 2.5f;
        
        // 绘制左球拍（3px高，亮度0x40）
        for (int d = -1; d <= 1; d++) {
            int py = (int)roundf(leftY) + d;
            if (py >= 0 && py < SCREEN_ROWS - 1)
                fb[py][1] = 0x40;
        }
        
        // 绘制右球拍
        for (int d = -1; d <= 1; d++) {
            int py = (int)roundf(rightY) + d;
            if (py >= 0 && py < SCREEN_ROWS - 1)
                fb[py][SCREEN_COLS - 2] = 0x40;
        }
        
        // 绘制球
        int bx = (int)roundf(ballX);
        int by = (int)roundf(ballY);
        if (bx >= 0 && bx < SCREEN_COLS && by >= 0 && by < SCREEN_ROWS - 1)
            fb[by][bx] = 0x40;
        
        // 绘制中线点
        for (int y = 0; y < SCREEN_ROWS - 1; y += 2)
            fb[y][(SCREEN_COLS / 2) - 1] = 0x40;
        
        // 绘制秒数进度条（底行）- 使用简单整数运算避免卡顿
        float pixelPos = totalSec * SCREEN_COLS / 60.0f;
        int fullPixels = (int)pixelPos;
        float fraction = pixelPos - fullPixels;
        uint8_t partialBri = (uint8_t)(fraction * 64);
        
        int barRow = SCREEN_ROWS - 1;
        for (int x = 0; x < SCREEN_COLS; x++) {
            if (x < fullPixels) {
                fb[barRow][x] = 0x40;
            } else if (x == fullPixels) {
                fb[barRow][x] = partialBri;
            } else {
                fb[barRow][x] = 0;
            }
        }
        
        // 绘制时钟显示 - 左右半屏分别居中
        // 左半屏显示小时（居中）
        char hourStr[3];
        snprintf(hourStr, sizeof(hourStr), "%02d", curHour);
        // 左半屏可用区域：x=2 到 x=19（18列），字体3x5，2位数字宽度=6
        // 居中位置：2 + (18 - 6) / 2 = 8
        // 垂直居中：(11 - 5) / 2 = 3
        FontRenderer::drawChar(hourStr[0], 8, 3, FontType::FONT_3x5, 0x60);
        FontRenderer::drawChar(hourStr[1], 11, 3, FontType::FONT_3x5, 0x60);
        
        // 右半屏显示分钟（居中）
        char minStr[3];
        snprintf(minStr, sizeof(minStr), "%02d", curMin);
        // 右半屏可用区域：x=20 到 x=37（18列），字体3x5，2位数字宽度=6
        // 居中位置：20 + (18 - 6) / 2 = 26
        FontRenderer::drawChar(minStr[0], 26, 3, FontType::FONT_3x5, 0x60);
        FontRenderer::drawChar(minStr[1], 29, 3, FontType::FONT_3x5, 0x60);
    }
    
    void update() override {
        // update逻辑已移到render中，通过lastUpdateMs控制帧率
    }
    
    bool canOverlay() const override { return false; }  // Pong不使用叠加
    
    SupportedPositions getSupportedPositions() const override {
        return {false, false, false, false, false, Position::CENTER};
    }

private:
    float leftY, rightY;
    float ballX, ballY;
    float ballVx, ballVy;
    int leftScore, rightScore;
    uint32_t baseMs;
    int lastSec;
    uint32_t lastUpdateMs;
};

#endif // PONG_BACKGROUND_H
