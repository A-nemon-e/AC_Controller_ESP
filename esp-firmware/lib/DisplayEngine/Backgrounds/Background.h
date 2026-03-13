/**
 * Background - 背景基类
 * 
 * 定义背景渲染接口，管理全屏动态效果
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <Arduino.h>
#include "../LEDMatrix/LEDMatrix.h"

// 位置枚举
enum class Position {
    TOP_LEFT,       // 左上
    TOP_CENTER,     // 上中
    TOP_RIGHT,      // 右上
    CENTER_LEFT,    // 左中
    CENTER,         // 中央
    CENTER_RIGHT,   // 右中
    BOTTOM_LEFT,    // 左下
    BOTTOM_CENTER,  // 下中
    BOTTOM_RIGHT    // 右下
};

// 前景位置支持
struct SupportedPositions {
    bool top;       // 顶部
    bool bottom;    // 底部
    bool left;      // 左侧
    bool right;     // 右侧
    bool center;    // 中央
    Position recommended;  // 推荐位置
};

class Background {
public:
    enum class Type {
        FIRE,           // 火焰
        MATRIX_RAIN,    // 矩阵雨
        WATER_RIPPLE,   // 水波纹
        GAME_OF_LIFE,   // 生命游戏
        SAND,           // 沙漏
        PONG,           // Pong时钟
        WEATHER_SUNNY,  // 晴天
        WEATHER_RAINY,  // 下雨
        WEATHER_SNOWY,  // 下雪
        WEATHER_WINDY,  // 刮风
        WEATHER_CLOUDY, // 阴天
        NONE            // 无背景
    };
    
    Background(Type type) : type(type), brightness(255), frameCount(0), 
                            lastUpdateTime(0), updateInterval(50) {}
    
    virtual ~Background() {}
    
    // 生命周期
    virtual void init() { frameCount = 0; lastUpdateTime = millis(); }
    virtual void reset() { init(); }
    
    // 渲染 - 纯虚函数
    virtual void render(LEDMatrix& matrix) = 0;
    
    // 更新动画状态
    virtual void update() {
        uint32_t now = millis();
        if (now - lastUpdateTime >= updateInterval) {
            lastUpdateTime = now;
            frameCount++;
        }
    }
    
    // 属性查询
    Type getType() const { return type; }
    virtual bool canOverlay() const { return true; }
    virtual SupportedPositions getSupportedPositions() const {
        return {true, true, true, true, true, Position::CENTER};
    }
    
    // 配置
    void setBrightness(uint8_t b) { brightness = b; }
    uint8_t getBrightness() const { return brightness; }
    
    void setUpdateInterval(uint16_t interval) { updateInterval = interval; }
    uint16_t getUpdateInterval() const { return updateInterval; }
    
    uint32_t getFrameCount() const { return frameCount; }

protected:
    Type type;
    uint8_t brightness;
    uint32_t frameCount;
    uint32_t lastUpdateTime;
    uint16_t updateInterval;  // 更新间隔(ms)
};

#endif // BACKGROUND_H
