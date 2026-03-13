/**
 * @file ButtonHandler.h
 * @brief 按键处理模块 - 支持短按、双击、长按检测
 * @details 使用轮询+状态机实现，适用于ESP8266的GPIO16（不支持中断）
 * @version 1.0.0
 * @date 2026-03-13
 * 
 * @note GPIO16特性：
 * - 不支持中断（硬件限制）
 * - 支持内部下拉（INPUT_PULLDOWN_16）
 * - 高电平有效（按下=HIGH，松开=LOW）
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include <functional>

// 按键引脚定义（与config_pins.h保持一致）
#ifndef PIN_BUTTON
#define PIN_BUTTON 16
#endif

// 按键状态
enum class ButtonState {
    IDLE,           // 空闲状态
    PRESSED,        // 按下中
    RELEASED,       // 已松开（等待确认单击/双击）
    LONG_PRESSING,  // 长按中
    LONG_PRESSED,   // 已触发长按
    BRIGHTNESS_MODE // 亮度调节模式
};

// 按键事件类型
enum class ButtonEvent {
    NONE,           // 无事件
    SINGLE_CLICK,   // 单击
    DOUBLE_CLICK,   // 双击
    LONG_PRESS,     // 长按开始
    LONG_PRESS_END, // 长按结束
    BRIGHTNESS_UP   // 亮度调节模式下短按
};

// 按键配置结构
struct ButtonConfig {
    uint16_t debounceMs;        // 消抖时间（毫秒）
    uint16_t clickTimeoutMs;    // 单击超时时间（毫秒）
    uint16_t doubleClickGapMs;  // 双击间隔最大时间（毫秒）
    uint16_t longPressMs;       // 长按触发时间（毫秒）
    uint16_t brightnessTimeoutMs; // 亮度调节模式超时（毫秒）
    
    // 默认配置
    static ButtonConfig getDefault() {
        return {
            .debounceMs = 50,           // 50ms消抖
            .clickTimeoutMs = 400,      // 400ms内无第二次点击视为单击
            .doubleClickGapMs = 400,    // 双击间隔最大400ms
            .longPressMs = 900,         // 900ms触发长按
            .brightnessTimeoutMs = 5000 // 亮度调节模式5秒超时
        };
    }
};

/**
 * @class ButtonHandler
 * @brief 按键处理类
 * 
 * 使用说明：
 * 1. 在setup()中调用 ButtonHandler::init()
 * 2. 在loop()中调用 ButtonHandler::update()
 * 3. 注册回调函数处理按键事件
 * 
 * 功能映射：
 * - 短按(<500ms): 亮屏/息屏切换
 * - 长按(>900ms): 进入亮度调节模式
 * - 长按中短按: 切换亮度级别
 * - 双击(<400ms间隔): 切换到下一张卡片
 */
class ButtonHandler {
public:
    /**
     * @brief 初始化按键处理模块
     * @param config 按键配置，不传则使用默认配置
     * @return true初始化成功，false失败
     */
    static bool init(const ButtonConfig& config = ButtonConfig::getDefault());
    
    /**
     * @brief 更新按键状态（需要在loop()中定期调用）
     * @details 建议调用频率：每10-20ms一次
     */
    static void update();
    
    /**
     * @brief 设置单击回调函数
     * @param callback 回调函数
     */
    static void onSingleClick(std::function<void()> callback);
    
    /**
     * @brief 设置双击回调函数
     * @param callback 回调函数
     */
    static void onDoubleClick(std::function<void()> callback);
    
    /**
     * @brief 设置长按开始回调函数
     * @param callback 回调函数
     */
    static void onLongPress(std::function<void()> callback);
    
    /**
     * @brief 设置长按结束回调函数
     * @param callback 回调函数
     */
    static void onLongPressEnd(std::function<void()> callback);
    
    /**
     * @brief 设置亮度调节模式下短按回调
     * @param callback 回调函数
     */
    static void onBrightnessClick(std::function<void()> callback);
    
    /**
     * @brief 获取当前按键状态
     * @return 按键状态
     */
    static ButtonState getState();
    
    /**
     * @brief 检查是否在亮度调节模式
     * @return true在亮度调节模式
     */
    static bool isBrightnessMode();
    
    /**
     * @brief 退出亮度调节模式
     */
    static void exitBrightnessMode();
    
    /**
     * @brief 获取按键按下时长（毫秒）
     * @return 按下时长，未按下返回0
     */
    static uint32_t getPressDuration();
    
    /**
     * @brief 检查按键是否被按下（原始状态）
     * @return true按键被按下
     */
    static bool isPressed();

private:
    // 配置
    static ButtonConfig config;
    
    // 状态
    static ButtonState state;
    static bool lastRawState;       // 上一次原始状态
    static bool debouncedState;     // 消抖后的状态
    static uint32_t lastDebounceTime;
    static uint32_t pressStartTime;
    static uint32_t releaseTime;
    static uint32_t lastUpdateTime;
    static uint8_t clickCount;      // 点击次数（用于双击检测）
    static uint32_t brightnessModeStartTime;
    
    // 回调函数
    static std::function<void()> singleClickCallback;
    static std::function<void()> doubleClickCallback;
    static std::function<void()> longPressCallback;
    static std::function<void()> longPressEndCallback;
    static std::function<void()> brightnessClickCallback;
    
    // 私有方法
    static bool readRawState();     // 读取原始按键状态
    static void updateDebounce();   // 更新消抖状态
    static void processStateMachine(); // 处理状态机
    static void triggerEvent(ButtonEvent event);
    static void enterBrightnessMode();
};

#endif // BUTTON_HANDLER_H