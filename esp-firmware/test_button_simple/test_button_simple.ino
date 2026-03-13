/**
 * @file test_button_simple.ino
 * @brief ButtonHandler模块简化测试程序
 * @details 不依赖LEDMatrix和FontRenderer，只测试按键功能
 */

#include <ButtonHandler.h>

// 测试状态
uint32_t lastEventTime = 0;
String lastEvent = "None";
uint32_t eventCount = 0;
bool ledState = false;

// 亮度级别（模拟）
const uint8_t BRIGHTNESS_LEVELS[] = {20, 60, 120, 200, 255};
const uint8_t NUM_BRIGHTNESS_LEVELS = 5;
uint8_t currentBrightnessIndex = 2;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("ButtonHandler Simple Test");
    Serial.println("========================================\n");
    
    // 配置LED（使用GPIO2，ESP8266内置LED）
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED off (active low)
    
    // 初始化按键处理
    ButtonConfig config = ButtonConfig::getDefault();
    if (!ButtonHandler::init(config)) {
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
    Serial.println("  - Single Click: Toggle LED");
    Serial.println("  - Double Click: Print 'Double Click'");
    Serial.println("  - Long Press: Enter brightness mode");
    Serial.println("  - Brightness Click: Cycle brightness levels");
    Serial.println("\nTest started! Press the button to test...\n");
}

void loop() {
    // 更新按键状态（必须定期调用）
    ButtonHandler::update();
    
    // 打印状态信息（每500ms）
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
    
    Serial.println("[EVENT] Single Click - Toggling LED");
    
    // 切换LED状态
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH); // Active low
    
    Serial.print("  LED: ");
    Serial.println(ledState ? "ON" : "OFF");
}

void onDoubleClick() {
    lastEvent = "DOUBLE CLICK";
    lastEventTime = millis();
    eventCount++;
    
    Serial.println("[EVENT] Double Click!");
    Serial.println("  (This would switch to next card in real app)");
}

void onLongPress() {
    lastEvent = "LONG PRESS";
    lastEventTime = millis();
    eventCount++;
    
    Serial.println("[EVENT] Long Press - Entering brightness mode");
    Serial.println("  Brightness mode active for 5 seconds");
    
    // 快速闪烁LED表示进入亮度模式
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
    }
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
    
    Serial.print("[EVENT] Brightness Click - Level: ");
    Serial.print(currentBrightnessIndex + 1);
    Serial.print("/");
    Serial.print(NUM_BRIGHTNESS_LEVELS);
    Serial.print(" (");
    Serial.print(newBrightness);
    Serial.println(")");
    
    // 用LED闪烁次数表示亮度级别
    for (int i = 0; i <= currentBrightnessIndex; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(150);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(150);
    }
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