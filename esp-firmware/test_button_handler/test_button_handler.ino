/**
 * @file test_button_handler.ino
 * @brief ButtonHandler模块测试程序
 * @details 测试按键的各种功能：短按、双击、长按、亮度调节模式
 */

#include <ButtonHandler.h>
#include <LEDMatrix.h>
#include <FontRenderer.h>

// 亮度级别
const uint8_t BRIGHTNESS_LEVELS[] = {20, 60, 120, 200, 255};
const uint8_t NUM_BRIGHTNESS_LEVELS = 5;
uint8_t currentBrightnessIndex = 2; // 默认120

// 测试状态
uint32_t lastEventTime = 0;
String lastEvent = "None";
uint32_t eventCount = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("ButtonHandler Test Program");
    Serial.println("========================================\n");
    
    // 初始化LED矩阵
    if (!LEDMatrix::begin()) {
        Serial.println("ERROR: LEDMatrix init failed!");
        return;
    }
    Serial.println("LEDMatrix initialized");
    
    // 设置初始亮度
    LEDMatrix::setGlobalBrightness(BRIGHTNESS_LEVELS[currentBrightnessIndex]);
    
    // 初始化按键处理
    if (!ButtonHandler::init()) {
        Serial.println("ERROR: ButtonHandler init failed!");
        return;
    }
    Serial.println("ButtonHandler initialized");
    
    // 注册回调函数
    ButtonHandler::onSingleClick(onSingleClick);
    ButtonHandler::onDoubleClick(onDoubleClick);
    ButtonHandler::onLongPress(onLongPress);
    ButtonHandler::onLongPressEnd(onLongPressEnd);
    ButtonHandler::onBrightnessClick(onBrightnessClick);
    
    Serial.println("\nCallbacks registered:");
    Serial.println("  - Single Click: Toggle screen on/off");
    Serial.println("  - Double Click: Switch to next card");
    Serial.println("  - Long Press: Enter brightness mode");
    Serial.println("  - Brightness Click: Cycle brightness levels");
    Serial.println("\nTest started! Press the button to test...\n");
    
    // 显示初始状态
    updateDisplay();
}

void loop() {
    // 更新按键状态（必须定期调用）
    ButtonHandler::update();
    
    // 更新显示（每100ms刷新一次）
    static uint32_t lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 100) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // 打印调试信息（每500ms）
    static uint32_t lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 500) {
        printDebugInfo();
        lastDebugPrint = millis();
    }
    
    delay(10); // 10ms更新周期
}

// 回调函数实现
void onSingleClick() {
    lastEvent = "SINGLE CLICK";
    lastEventTime = millis();
    eventCount++;
    
    Serial.println("[EVENT] Single Click - Toggling screen");
    
    // 切换屏幕开关
    static bool screenOn = true;
    screenOn = !screenOn;
    LEDMatrix::setScreenOn(screenOn);
    
    Serial.print("  Screen: ");
    Serial.println(screenOn ? "ON" : "OFF");
}

void onDoubleClick() {
    lastEvent = "DOUBLE CLICK";
    lastEventTime = millis();
    eventCount++;
    
    Serial.println("[EVENT] Double Click - Switching card");
    Serial.println("  (This would switch to next card in real app)");
    
    // 测试：显示不同内容
    static uint8_t testMode = 0;
    testMode = (testMode + 1) % 3;
    
    LEDMatrix::clear();
    switch (testMode) {
        case 0:
            FontRenderer::drawString("CARD1", 8, 3, FontType::FONT_5x7, 255);
            break;
        case 1:
            FontRenderer::drawString("CARD2", 8, 3, FontType::FONT_5x7, 255);
            break;
        case 2:
            FontRenderer::drawString("CARD3", 8, 3, FontType::FONT_5x7, 255);
            break;
    }
    LEDMatrix::refresh();
}

void onLongPress() {
    lastEvent = "LONG PRESS";
    lastEventTime = millis();
    eventCount++;
    
    Serial.println("[EVENT] Long Press - Entering brightness mode");
    Serial.println("  Brightness mode active for 5 seconds");
    
    // 显示亮度模式提示
    LEDMatrix::clear();
    FontRenderer::drawString("BRIGHT", 6, 2, FontType::FONT_5x7, 255);
    LEDMatrix::refresh();
}

void onLongPressEnd() {
    lastEvent = "LONG PRESS END";
    lastEventTime = millis();
    
    Serial.println("[EVENT] Long Press End");
}

void onBrightnessClick() {
    lastEvent = "BRIGHTNESS CLICK";
    lastEventTime = millis();
    eventCount++;
    
    // 切换亮度级别
    currentBrightnessIndex = (currentBrightnessIndex + 1) % NUM_BRIGHTNESS_LEVELS;
    uint8_t newBrightness = BRIGHTNESS_LEVELS[currentBrightnessIndex];
    
    LEDMatrix::setGlobalBrightness(newBrightness);
    
    Serial.print("[EVENT] Brightness Click - Level: ");
    Serial.print(currentBrightnessIndex + 1);
    Serial.print("/");
    Serial.print(NUM_BRIGHTNESS_LEVELS);
    Serial.print(" (");
    Serial.print(newBrightness);
    Serial.println(")");
    
    // 显示当前亮度
    LEDMatrix::clear();
    char buf[16];
    sprintf(buf, "BRI %d", newBrightness);
    FontRenderer::drawString(buf, 6, 3, FontType::FONT_5x7, 255);
    LEDMatrix::refresh();
}

// 更新显示
void updateDisplay() {
    // 只在亮度调节模式下更新显示
    if (!ButtonHandler::isBrightnessMode()) {
        return;
    }
    
    // 显示剩余时间
    uint32_t elapsed = millis() - lastEventTime;
    uint32_t remaining = (elapsed < 5000) ? (5000 - elapsed) / 1000 : 0;
    
    LEDMatrix::clear();
    
    // 显示亮度值
    char buf[16];
    sprintf(buf, "BRI %d", BRIGHTNESS_LEVELS[currentBrightnessIndex]);
    FontRenderer::drawString(buf, 6, 1, FontType::FONT_5x7, 255);
    
    // 显示倒计时
    sprintf(buf, "T-%ds", remaining);
    FontRenderer::drawString(buf, 10, 7, FontType::FONT_3x5, 200);
    
    LEDMatrix::refresh();
}

// 打印调试信息
void printDebugInfo() {
    Serial.print("State: ");
    switch (ButtonHandler::getState()) {
        case ButtonState::IDLE:
            Serial.print("IDLE");
            break;
        case ButtonState::PRESSED:
            Serial.print("PRESSED");
            break;
        case ButtonState::RELEASED:
            Serial.print("RELEASED");
            break;
        case ButtonState::LONG_PRESSING:
            Serial.print("LONG_PRESSING");
            break;
        case ButtonState::LONG_PRESSED:
            Serial.print("LONG_PRESSED");
            break;
        case ButtonState::BRIGHTNESS_MODE:
            Serial.print("BRIGHTNESS_MODE");
            break;
    }
    
    Serial.print(" | Pressed: ");
    Serial.print(ButtonHandler::isPressed() ? "YES" : "NO");
    
    Serial.print(" | Duration: ");
    Serial.print(ButtonHandler::getPressDuration());
    Serial.print("ms");
    
    Serial.print(" | Events: ");
    Serial.print(eventCount);
    
    Serial.print(" | Last: ");
    Serial.println(lastEvent);
}