/**
 * @file ButtonHandler.cpp
 * @brief 按键处理模块实现
 * @details 使用轮询+状态机实现按键检测
 */

#include "ButtonHandler.h"

// 静态成员初始化
ButtonConfig ButtonHandler::config;
ButtonState ButtonHandler::state = ButtonState::IDLE;
bool ButtonHandler::lastRawState = false;
bool ButtonHandler::debouncedState = false;
uint32_t ButtonHandler::lastDebounceTime = 0;
uint32_t ButtonHandler::pressStartTime = 0;
uint32_t ButtonHandler::releaseTime = 0;
uint32_t ButtonHandler::lastUpdateTime = 0;
uint8_t ButtonHandler::clickCount = 0;
uint32_t ButtonHandler::brightnessModeStartTime = 0;

// 回调函数初始化
std::function<void()> ButtonHandler::singleClickCallback = nullptr;
std::function<void()> ButtonHandler::doubleClickCallback = nullptr;
std::function<void()> ButtonHandler::longPressCallback = nullptr;
std::function<void()> ButtonHandler::longPressEndCallback = nullptr;
std::function<void()> ButtonHandler::brightnessClickCallback = nullptr;

bool ButtonHandler::init(const ButtonConfig& cfg) {
    config = cfg;
    
    // 配置GPIO16为输入，启用内部下拉
    // ESP8266的GPIO16支持INPUT_PULLDOWN_16
    pinMode(PIN_BUTTON, INPUT_PULLDOWN_16);
    
    // 初始化状态
    state = ButtonState::IDLE;
    lastRawState = false;
    debouncedState = false;
    clickCount = 0;
    
    // 读取初始状态
    lastRawState = readRawState();
    debouncedState = lastRawState;
    
    lastUpdateTime = millis();
    
    return true;
}

void ButtonHandler::update() {
    uint32_t currentTime = millis();
    
    // 防止millis()溢出（虽然需要49天）
    if (currentTime < lastUpdateTime) {
        // 重置所有时间基准
        lastDebounceTime = 0;
        pressStartTime = 0;
        releaseTime = 0;
        brightnessModeStartTime = 0;
    }
    lastUpdateTime = currentTime;
    
    // 更新消抖状态
    updateDebounce();
    
    // 处理状态机
    processStateMachine();
}

bool ButtonHandler::readRawState() {
    // GPIO16高电平有效（按下=HIGH，松开=LOW）
    return digitalRead(PIN_BUTTON) == HIGH;
}

void ButtonHandler::updateDebounce() {
    bool currentRawState = readRawState();
    
    // 如果状态改变，重置消抖计时器
    if (currentRawState != lastRawState) {
        lastDebounceTime = millis();
    }
    
    // 如果状态稳定超过消抖时间，更新消抖后状态
    if ((millis() - lastDebounceTime) > config.debounceMs) {
        debouncedState = currentRawState;
    }
    
    lastRawState = currentRawState;
}

void ButtonHandler::processStateMachine() {
    uint32_t currentTime = millis();
    bool isButtonPressed = debouncedState;
    
    switch (state) {
        case ButtonState::IDLE:
            if (isButtonPressed) {
                // 按键被按下，这是第一次点击
                pressStartTime = currentTime;
                clickCount = 1;  // 修复：第一次按下时设置clickCount为1
                state = ButtonState::PRESSED;
            }
            break;
            
        case ButtonState::PRESSED:
            if (!isButtonPressed) {
                // 按键松开，可能是单击或双击的开始
                releaseTime = currentTime;
                state = ButtonState::RELEASED;
            } else if ((currentTime - pressStartTime) >= config.longPressMs) {
                // 达到长按时间，触发长按
                state = ButtonState::LONG_PRESSING;
                triggerEvent(ButtonEvent::LONG_PRESS);
            }
            break;
            
        case ButtonState::RELEASED:
            if (isButtonPressed) {
                // 在超时前再次按下，是双击
                if ((currentTime - releaseTime) <= config.doubleClickGapMs) {
                    clickCount = 2;  // 修复：明确设置为2表示双击
                    pressStartTime = currentTime;
                    state = ButtonState::PRESSED;
                } else {
                    // 超时后按下，是新的单击序列
                    clickCount = 1;
                    pressStartTime = currentTime;
                    state = ButtonState::PRESSED;
                }
            } else if ((currentTime - releaseTime) > config.clickTimeoutMs) {
                // 超时，根据clickCount判断是单击还是双击
                // 修复：clickCount == 2表示双击，clickCount == 1表示单击
                if (clickCount == 2) {
                    triggerEvent(ButtonEvent::DOUBLE_CLICK);
                } else {
                    triggerEvent(ButtonEvent::SINGLE_CLICK);
                }
                clickCount = 0;
                state = ButtonState::IDLE;
            }
            break;
            
        case ButtonState::LONG_PRESSING:
            if (!isButtonPressed) {
                // 长按结束
                triggerEvent(ButtonEvent::LONG_PRESS_END);
                state = ButtonState::IDLE;
            }
            break;
            
        case ButtonState::LONG_PRESSED:
            // 这个状态在触发长按后进入亮度调节模式
            if (!isButtonPressed) {
                state = ButtonState::BRIGHTNESS_MODE;
                brightnessModeStartTime = currentTime;
            }
            break;
            
        case ButtonState::BRIGHTNESS_MODE:
            // 检查是否超时
            if ((currentTime - brightnessModeStartTime) >= config.brightnessTimeoutMs) {
                exitBrightnessMode();
                return;
            }
            
            if (isButtonPressed) {
                // 在亮度调节模式下按下
                pressStartTime = currentTime;
                state = ButtonState::PRESSED;
            }
            break;
    }
}

void ButtonHandler::triggerEvent(ButtonEvent event) {
    switch (event) {
        case ButtonEvent::SINGLE_CLICK:
            if (singleClickCallback) {
                singleClickCallback();
            }
            break;
            
        case ButtonEvent::DOUBLE_CLICK:
            if (doubleClickCallback) {
                doubleClickCallback();
            }
            break;
            
        case ButtonEvent::LONG_PRESS:
            if (longPressCallback) {
                longPressCallback();
            }
            // 触发长按后进入亮度调节模式
            state = ButtonState::LONG_PRESSED;
            brightnessModeStartTime = millis();
            break;
            
        case ButtonEvent::LONG_PRESS_END:
            if (longPressEndCallback) {
                longPressEndCallback();
            }
            break;
            
        case ButtonEvent::BRIGHTNESS_UP:
            if (brightnessClickCallback) {
                brightnessClickCallback();
            }
            // 重置亮度调节模式超时
            brightnessModeStartTime = millis();
            break;
            
        default:
            break;
    }
}

void ButtonHandler::enterBrightnessMode() {
    state = ButtonState::BRIGHTNESS_MODE;
    brightnessModeStartTime = millis();
}

void ButtonHandler::exitBrightnessMode() {
    state = ButtonState::IDLE;
    clickCount = 0;
}

// 回调函数设置
void ButtonHandler::onSingleClick(std::function<void()> callback) {
    singleClickCallback = callback;
}

void ButtonHandler::onDoubleClick(std::function<void()> callback) {
    doubleClickCallback = callback;
}

void ButtonHandler::onLongPress(std::function<void()> callback) {
    longPressCallback = callback;
}

void ButtonHandler::onLongPressEnd(std::function<void()> callback) {
    longPressEndCallback = callback;
}

void ButtonHandler::onBrightnessClick(std::function<void()> callback) {
    brightnessClickCallback = callback;
}

// 状态查询
ButtonState ButtonHandler::getState() {
    return state;
}

bool ButtonHandler::isBrightnessMode() {
    return state == ButtonState::BRIGHTNESS_MODE;
}

uint32_t ButtonHandler::getPressDuration() {
    if (state == ButtonState::PRESSED || state == ButtonState::LONG_PRESSING) {
        return millis() - pressStartTime;
    }
    return 0;
}

bool ButtonHandler::isPressed() {
    return debouncedState;
}