/**
 * DisplayManager - 显示管理器
 * 
 * 管理所有卡片、处理切换、协调渲染
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "Card.h"
#include "Transition.h"
#include "../LEDMatrix/LEDMatrix.h"
#include "../FontRenderer/FontRenderer.h"

class DisplayManager {
public:
    static const size_t MAX_CARDS = 10;
    static const uint16_t DEFAULT_SWITCH_INTERVAL = 30; // 30秒
    static const uint16_t DEFAULT_TRANSITION_DURATION = 500; // 500ms
    
    static DisplayManager& getInstance() {
        static DisplayManager instance;
        return instance;
    }
    
    // 初始化
    void init() {
        cardCount = 0;
        currentCardIndex = 0;
        autoSwitchCards = false;
        cardSwitchInterval = DEFAULT_SWITCH_INTERVAL * 1000;
        cardStartTime = millis();
        lastRenderTime = 0;
        frameCount = 0;
        currentFPS = 0;
        transition.setDuration(DEFAULT_TRANSITION_DURATION);
        
        memset(cards, 0, sizeof(cards));
    }
    
    void begin() {
        LEDMatrix::begin();
        init();
    }
    
    // 卡片管理
    bool addCard(Card* card) {
        if (cardCount >= MAX_CARDS || !card) {
            return false;
        }
        cards[cardCount++] = card;
        return true;
    }
    
    void removeCard(size_t index) {
        if (index < cardCount) {
            delete cards[index];
            for (size_t i = index; i < cardCount - 1; i++) {
                cards[i] = cards[i + 1];
            }
            cards[--cardCount] = nullptr;
            
            // 调整当前索引
            if (currentCardIndex >= cardCount && cardCount > 0) {
                currentCardIndex = cardCount - 1;
            }
        }
    }
    
    void clearCards() {
        for (size_t i = 0; i < cardCount; i++) {
            delete cards[i];
            cards[i] = nullptr;
        }
        cardCount = 0;
        currentCardIndex = 0;
    }
    
    size_t getCardCount() const { return cardCount; }
    
    Card* getCard(size_t index) const {
        if (index < cardCount) {
            return cards[index];
        }
        return nullptr;
    }
    
    Card* getCurrentCard() const {
        if (currentCardIndex < cardCount) {
            return cards[currentCardIndex];
        }
        return nullptr;
    }
    
    // 卡片切换
    void switchCard(size_t index) {
        if (index >= cardCount || index == currentCardIndex) {
            return;
        }
        
        // 准备转场
        if (transition.getType() != TransitionType::INSTANT) {
            // 渲染当前卡片到Buffer A
            if (getCurrentCard()) {
                LEDMatrix::clear();
                getCurrentCard()->render(getMatrix(), fonts);
                memcpy(transitionBufferA, LEDMatrix::getBuffer(), 
                       SCREEN_ROWS * SCREEN_COLS);
            }
            
            // 设置新卡片索引
            previousCardIndex = currentCardIndex;
            currentCardIndex = index;
            cardStartTime = millis();
            
            // 渲染新卡片到Buffer B
            if (getCurrentCard()) {
                LEDMatrix::clear();
                getCurrentCard()->render(getMatrix(), fonts);
                memcpy(transitionBufferB, LEDMatrix::getBuffer(),
                       SCREEN_ROWS * SCREEN_COLS);
            }
            
            // 开始转场
            transition.begin();
        } else {
            // 直接切换
            currentCardIndex = index;
            cardStartTime = millis();
        }
    }
    
    void nextCard() {
        if (cardCount > 0) {
            switchCard((currentCardIndex + 1) % cardCount);
        }
    }
    
    void previousCard() {
        if (cardCount > 0) {
            switchCard((currentCardIndex + cardCount - 1) % cardCount);
        }
    }
    
    void setAutoSwitch(bool enable) { autoSwitchCards = enable; }
    bool getAutoSwitch() const { return autoSwitchCards; }
    
    void setSwitchInterval(uint16_t seconds) { 
        cardSwitchInterval = seconds * 1000; 
    }
    uint16_t getSwitchInterval() const { return cardSwitchInterval / 1000; }
    
    // 按键处理（集成）
    void onButtonClick() {
        // 单击：切换下一张卡片
        nextCard();
    }
    
    void onButtonDoubleClick() {
        // 双击：切换自动/手动模式
        autoSwitchCards = !autoSwitchCards;
    }
    
    void onButtonLongPress() {
        // 长按：显示卡片内下一个前景
        if (getCurrentCard()) {
            getCurrentCard()->nextForeground();
        }
    }
    
    // 主循环
    void update() {
        // 更新转场
        if (transition.isInProgress()) {
            transition.update();
            if (transition.isFinished()) {
                // 转场完成，清屏准备正常渲染
                LEDMatrix::clear();
            }
        }
        
        // 更新当前卡片
        if (getCurrentCard()) {
            getCurrentCard()->update();
        }
        
        // 自动切换卡片
        if (autoSwitchCards && !transition.isInProgress() && cardCount > 1) {
            if (millis() - cardStartTime >= cardSwitchInterval) {
                nextCard();
            }
        }
    }
    
    void render() {
        uint32_t now = millis();
        
        // FPS计算
        if (now - lastRenderTime >= 1000) {
            currentFPS = frameCount;
            frameCount = 0;
            lastRenderTime = now;
        }
        
        if (transition.isInProgress()) {
            // 使用转场效果混合
            uint8_t outputBuffer[SCREEN_ROWS][SCREEN_COLS];
            transition.blend(transitionBufferA, transitionBufferB, outputBuffer);
            
            // 复制到LEDMatrix缓冲区
            memcpy(LEDMatrix::getBuffer(), outputBuffer, SCREEN_ROWS * SCREEN_COLS);
        } else {
            // 正常渲染当前卡片
            if (getCurrentCard()) {
                getCurrentCard()->render(getMatrix(), fonts);
            } else {
                LEDMatrix::clear();
            }
        }
        
        // 刷新到硬件
        LEDMatrix::refresh();
        frameCount++;
    }
    
    // 设置转场效果
    void setTransitionType(TransitionType type) {
        transition.setType(type);
    }
    
    void setTransitionDuration(uint16_t ms) {
        transition.setDuration(ms);
    }
    
    // 获取统计
    uint8_t getFPS() const { return currentFPS; }
    uint32_t getFrameCount() const { return frameCount; }

private:
    DisplayManager() {}
    ~DisplayManager() {
        clearCards();
    }
    
    // 禁止拷贝
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    
    LEDMatrix& getMatrix() {
        return matrix;
    }
    
    Card* cards[MAX_CARDS];
    size_t cardCount;
    size_t currentCardIndex;
    size_t previousCardIndex;
    
    // 自动切换
    bool autoSwitchCards;
    uint32_t cardSwitchInterval;
    uint32_t cardStartTime;
    
    // 转场
    Transition transition;
    uint8_t transitionBufferA[SCREEN_ROWS][SCREEN_COLS];
    uint8_t transitionBufferB[SCREEN_ROWS][SCREEN_COLS];
    
    // 硬件接口
    LEDMatrix matrix;
    FontRenderer fonts;
    
    // 渲染统计
    uint32_t frameCount;
    uint32_t lastRenderTime;
    uint8_t currentFPS;
};

#endif // DISPLAY_MANAGER_H
