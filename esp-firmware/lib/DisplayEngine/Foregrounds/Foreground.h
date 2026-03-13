/**
 * Foreground - 前景基类
 * 
 * 定义前景渲染接口，管理信息叠加显示
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef FOREGROUND_H
#define FOREGROUND_H

#include <Arduino.h>
#include "../LEDMatrix/LEDMatrix.h"
#include "../FontRenderer/FontRenderer.h"
#include "../Backgrounds/Background.h"

class Foreground {
public:
    enum class Type {
        CLOCK,          // 时钟（时:分 或 时:分:秒）
        DATE,           // 日期（月-日）
        WEEKDAY,        // 星期
        TEMP_HUMID,     // 温湿度
        NONE
    };
    
    Foreground(Type type) : type(type), position(Position::CENTER), 
                           font(FontType::FONT_5x7), brightness(255),
                           displayDuration(0), displayStartTime(0) {}
    
    virtual ~Foreground() {}
    
    // 生命周期
    virtual void init() { displayStartTime = millis(); }
    virtual void reset() { init(); }
    
    // 渲染 - 纯虚函数
    virtual void render(LEDMatrix& matrix, FontRenderer& fonts) = 0;
    
    // 更新
    virtual void update() {}
    
    // 尺寸计算 - 纯虚函数
    virtual void getSize(uint8_t& w, uint8_t& h) const = 0;
    
    // 位置管理
    void setPosition(Position pos) { position = pos; }
    Position getPosition() const { return position; }
    
    // 字体设置
    void setFont(FontType f) { font = f; }
    FontType getFont() const { return font; }
    
    // 亮度
    void setBrightness(uint8_t b) { brightness = b; }
    uint8_t getBrightness() const { return brightness; }
    
    // 自动切换时间（0=不自动切换）
    void setDisplayDuration(uint16_t seconds) { displayDuration = seconds * 1000; }
    uint16_t getDisplayDuration() const { return displayDuration / 1000; }
    
    // 检查是否应该切换到下一个前景
    bool shouldSwitch() const {
        if (displayDuration == 0) return false;
        return (millis() - displayStartTime) >= displayDuration;
    }
    
    void resetDisplayTimer() { displayStartTime = millis(); }
    
    Type getType() const { return type; }

protected:
    // 计算实际坐标
    void calculatePosition(uint8_t& x, uint8_t& y, uint8_t contentW, uint8_t contentH) const {
        switch (position) {
            case Position::TOP_LEFT:
                x = 0; y = 0;
                break;
            case Position::TOP_CENTER:
                x = (SCREEN_COLS - contentW) / 2; y = 0;
                break;
            case Position::TOP_RIGHT:
                x = SCREEN_COLS - contentW; y = 0;
                break;
            case Position::CENTER_LEFT:
                x = 0; y = (SCREEN_ROWS - contentH) / 2;
                break;
            case Position::CENTER:
                x = (SCREEN_COLS - contentW) / 2;
                y = (SCREEN_ROWS - contentH) / 2;
                break;
            case Position::CENTER_RIGHT:
                x = SCREEN_COLS - contentW; y = (SCREEN_ROWS - contentH) / 2;
                break;
            case Position::BOTTOM_LEFT:
                x = 0; y = SCREEN_ROWS - contentH;
                break;
            case Position::BOTTOM_CENTER:
                x = (SCREEN_COLS - contentW) / 2; y = SCREEN_ROWS - contentH;
                break;
            case Position::BOTTOM_RIGHT:
                x = SCREEN_COLS - contentW; y = SCREEN_ROWS - contentH;
                break;
        }
    }
    
    Type type;
    Position position;
    FontType font;
    uint8_t brightness;
    uint32_t displayDuration;  // 显示时长(ms)
    uint32_t displayStartTime; // 开始显示时间
};

#endif // FOREGROUND_H
