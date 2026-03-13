/**
 * Card - 卡片类
 * 
 * 组合0-1个背景和0-N个前景，管理显示逻辑
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef CARD_H
#define CARD_H

#include <Arduino.h>
#include "../Backgrounds/Background.h"
#include "../Foregrounds/Foreground.h"
#include "../FontRenderer/FontRenderer.h"

class Card {
public:
    static const size_t MAX_FOREGROUNDS = 5;
    
    Card(const char* name = "Unnamed") : background(nullptr), 
                                          foregroundCount(0),
                                          autoSwitchForegrounds(false),
                                          currentForegroundIndex(0),
                                          foregroundStartTime(0) {
        strncpy(this->name, name, 31);
        this->name[31] = '\0';
        memset(foregrounds, 0, sizeof(foregrounds));
    }
    
    ~Card() {
        removeBackground();
        clearForegrounds();
    }
    
    // 背景管理
    void setBackground(Background* bg) {
        background = bg;
    }
    
    Background* getBackground() const { 
        return background; 
    }
    
    void removeBackground() {
        if (background) {
            delete background;
            background = nullptr;
        }
    }
    
    // 前景管理
    bool addForeground(Foreground* fg) {
        if (foregroundCount >= MAX_FOREGROUNDS || !fg) {
            return false;
        }
        
        // 验证前景位置与背景兼容
        if (background && !validateForegroundPosition(fg)) {
            // 使用背景推荐位置
            SupportedPositions pos = background->getSupportedPositions();
            fg->setPosition(pos.recommended);
        }
        
        fg->resetDisplayTimer();
        foregrounds[foregroundCount++] = fg;
        return true;
    }
    
    void removeForeground(size_t index) {
        if (index < foregroundCount) {
            delete foregrounds[index];
            // 移动后面的元素
            for (size_t i = index; i < foregroundCount - 1; i++) {
                foregrounds[i] = foregrounds[i + 1];
            }
            foregrounds[--foregroundCount] = nullptr;
        }
    }
    
    void clearForegrounds() {
        for (size_t i = 0; i < foregroundCount; i++) {
            delete foregrounds[i];
            foregrounds[i] = nullptr;
        }
        foregroundCount = 0;
    }
    
    size_t getForegroundCount() const { return foregroundCount; }
    
    Foreground* getForeground(size_t index) const {
        if (index < foregroundCount) {
            return foregrounds[index];
        }
        return nullptr;
    }
    
    Foreground* getCurrentForeground() const {
        if (foregroundCount == 0) return nullptr;
        return foregrounds[currentForegroundIndex];
    }
    
    // 渲染
    void render(LEDMatrix& matrix, FontRenderer& fonts) {
        // 1. 渲染背景
        if (background) {
            background->render(matrix);
        } else {
            matrix.clear();
        }
        
        // 2. 渲染前景
        if (foregroundCount > 0) {
            Foreground* fg = foregrounds[currentForegroundIndex];
            if (fg) {
                fg->render(matrix, fonts);
            }
        }
    }
    
    void update() {
        // 更新背景
        if (background) {
            background->update();
        }
        
        // 更新前景
        if (foregroundCount > 0) {
            Foreground* fg = foregrounds[currentForegroundIndex];
            if (fg) {
                fg->update();
                
                // 检查是否需要切换前景
                if (autoSwitchForegrounds && fg->shouldSwitch()) {
                    nextForeground();
                }
            }
        }
    }
    
    // 前景切换
    void nextForeground() {
        if (foregroundCount > 0) {
            currentForegroundIndex = (currentForegroundIndex + 1) % foregroundCount;
            foregroundStartTime = millis();
            if (foregrounds[currentForegroundIndex]) {
                foregrounds[currentForegroundIndex]->resetDisplayTimer();
            }
        }
    }
    
    void previousForeground() {
        if (foregroundCount > 0) {
            currentForegroundIndex = (currentForegroundIndex + foregroundCount - 1) % foregroundCount;
            foregroundStartTime = millis();
            if (foregrounds[currentForegroundIndex]) {
                foregrounds[currentForegroundIndex]->resetDisplayTimer();
            }
        }
    }
    
    void setAutoSwitch(bool enable) { autoSwitchForegrounds = enable; }
    bool getAutoSwitch() const { return autoSwitchForegrounds; }
    
    void setCurrentForeground(size_t index) {
        if (index < foregroundCount) {
            currentForegroundIndex = index;
            foregroundStartTime = millis();
            if (foregrounds[currentForegroundIndex]) {
                foregrounds[currentForegroundIndex]->resetDisplayTimer();
            }
        }
    }
    
    size_t getCurrentForegroundIndex() const { return currentForegroundIndex; }
    
    // 名称
    const char* getName() const { return name; }
    void setName(const char* newName) {
        strncpy(name, newName, 31);
        name[31] = '\0';
    }

private:
    char name[32];
    Background* background;
    Foreground* foregrounds[MAX_FOREGROUNDS];
    size_t foregroundCount;
    
    bool autoSwitchForegrounds;
    size_t currentForegroundIndex;
    uint32_t foregroundStartTime;
    
    // 验证前景与背景的兼容性
    bool validateForegroundPosition(Foreground* fg) const {
        if (!background || !fg) return true;
        
        SupportedPositions pos = background->getSupportedPositions();
        Position fgPos = fg->getPosition();
        
        switch (fgPos) {
            case Position::TOP_LEFT:
            case Position::TOP_CENTER:
            case Position::TOP_RIGHT:
                return pos.top;
            case Position::BOTTOM_LEFT:
            case Position::BOTTOM_CENTER:
            case Position::BOTTOM_RIGHT:
                return pos.bottom;
            case Position::CENTER_LEFT:
            case Position::CENTER_RIGHT:
                return pos.left || pos.right;
            case Position::CENTER:
                return pos.center;
        }
        return true;
    }
};

#endif // CARD_H
