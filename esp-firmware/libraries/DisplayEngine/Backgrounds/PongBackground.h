/**
 * PongBackground - Pong游戏时钟背景
 * 
 * 经典Pong游戏作为时钟显示
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef PONG_BACKGROUND_H
#define PONG_BACKGROUND_H

#include "../Backgrounds/Background.h"

class PongBackground : public Background {
public:
    PongBackground() : Background(Type::PONG) {
        updateInterval = 40;  // 25fps
        ballX = SCREEN_COLS / 2;
        ballY = SCREEN_ROWS / 2;
        ballDX = 1;
        ballDY = 1;
        paddle1Y = SCREEN_ROWS / 2 - 2;
        paddle2Y = SCREEN_ROWS / 2 - 2;
        score1 = 0;
        score2 = 0;
        paddleHeight = 4;
    }
    
    void init() override {
        Background::init();
        resetBall();
        paddle1Y = SCREEN_ROWS / 2 - paddleHeight / 2;
        paddle2Y = SCREEN_ROWS / 2 - paddleHeight / 2;
        score1 = 0;
        score2 = 0;
    }
    
    void render(LEDMatrix& matrix) override {
        matrix.clear();
        
        // 绘制中线
        for (uint8_t y = 0; y < SCREEN_ROWS; y += 2) {
            matrix.setPixel(SCREEN_COLS / 2, y, brightness / 2);
        }
        
        // 绘制球拍
        for (uint8_t i = 0; i < paddleHeight; i++) {
            matrix.setPixel(1, paddle1Y + i, brightness);
            matrix.setPixel(SCREEN_COLS - 2, paddle2Y + i, brightness);
        }
        
        // 绘制球
        matrix.setPixel((uint8_t)ballX, (uint8_t)ballY, brightness);
        
        // 更新球位置
        updateBall();
    }
    
    void update() override {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
            updateBall();
            updatePaddles();
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, false, false, false, false, Position::TOP_CENTER};
    }

private:
    float ballX, ballY;
    float ballDX, ballDY;
    uint8_t paddle1Y, paddle2Y;
    uint8_t score1, score2;
    uint8_t paddleHeight;
    
    void resetBall() {
        ballX = SCREEN_COLS / 2;
        ballY = SCREEN_ROWS / 2;
        ballDX = random(2) == 0 ? 0.5 : -0.5;
        ballDY = (random(100) - 50) / 100.0;
    }
    
    void updateBall() {
        ballX += ballDX;
        ballY += ballDY;
        
        // 上下边界反弹
        if (ballY <= 0 || ballY >= SCREEN_ROWS - 1) {
            ballDY = -ballDY;
            ballY = constrainFloat(ballY, 0, SCREEN_ROWS - 1);
        }
        
        // 左球拍碰撞
        if (ballX <= 2 && ballY >= paddle1Y && ballY < paddle1Y + paddleHeight) {
            ballDX = abs(ballDX);
            ballDX *= 1.05;  // 加速
            ballDY += (ballY - (paddle1Y + paddleHeight / 2)) * 0.1;
        }
        
        // 右球拍碰撞
        if (ballX >= SCREEN_COLS - 3 && ballY >= paddle2Y && ballY < paddle2Y + paddleHeight) {
            ballDX = -abs(ballDX);
            ballDX *= 1.05;  // 加速
            ballDY += (ballY - (paddle2Y + paddleHeight / 2)) * 0.1;
        }
        
        // 得分检测
        if (ballX < 0) {
            score2++;
            resetBall();
        } else if (ballX >= SCREEN_COLS) {
            score1++;
            resetBall();
        }
    }
    
    void updatePaddles() {
        // AI控制球拍
        // 左球拍追踪球
        if (ballDX < 0) {
            if (ballY < paddle1Y + 1 && paddle1Y > 0) {
                paddle1Y--;
            } else if (ballY > paddle1Y + paddleHeight - 2 && paddle1Y + paddleHeight < SCREEN_ROWS) {
                paddle1Y++;
            }
        }
        
        // 右球拍追踪球
        if (ballDX > 0) {
            if (ballY < paddle2Y + 1 && paddle2Y > 0) {
                paddle2Y--;
            } else if (ballY > paddle2Y + paddleHeight - 2 && paddle2Y + paddleHeight < SCREEN_ROWS) {
                paddle2Y++;
            }
        }
    }
    
    float constrainFloat(float val, float min, float max) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }
};

#endif // PONG_BACKGROUND_H
