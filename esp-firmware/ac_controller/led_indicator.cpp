/*
 * LED指示器模块 - 实现
 */

#include "led_indicator.h"

// 静态成员初始化
SystemStatus LEDIndicator::currentStatus = STATUS_UNCONFIGURED;
unsigned long LEDIndicator::lastBlinkTime = 0;
bool LEDIndicator::ledState = false;
unsigned long LEDIndicator::irBlinkStart = 0;

void LEDIndicator::init() {
  pinMode(PIN_LED_SYS, OUTPUT);
  pinMode(PIN_LED_IR, OUTPUT);

  // 初始状态：系统LED灭，红外LED灭
  setSystemLED(false);
  setIRLED(false);

  DEBUG_PRINTLN("[LED] 初始化完成");
}

void LEDIndicator::update() {
  unsigned long now = millis();

  // ===== 系统LED状态控制 =====
  switch (currentStatus) {
  case STATUS_UNCONFIGURED:
    // 快闪200ms
    if (now - lastBlinkTime > 200) {
      ledState = !ledState;
      setSystemLED(ledState);
      lastBlinkTime = now;
    }
    break;

  case STATUS_WIFI_CONNECTING:
    // 慢闪1000ms
    if (now - lastBlinkTime > 1000) {
      ledState = !ledState;
      setSystemLED(ledState);
      lastBlinkTime = now;
    }
    break;

  case STATUS_MQTT_CONNECTING:
    // 快闪500ms
    if (now - lastBlinkTime > 500) {
      ledState = !ledState;
      setSystemLED(ledState);
      lastBlinkTime = now;
    }
    break;

  case STATUS_READY:
    // 常亮
    setSystemLED(true);
    break;
  }

  // ===== 红外LED控制（100ms后自动熄灭）=====
  if (irBlinkStart > 0 && now - irBlinkStart > 100) {
    setIRLED(false);
    irBlinkStart = 0;
  }
}

void LEDIndicator::setStatus(SystemStatus status) {
  if (currentStatus != status) {
    currentStatus = status;
    lastBlinkTime = millis();
    ledState = false;

    DEBUG_PRINT("[LED] 状态切换: ");
    switch (status) {
    case STATUS_UNCONFIGURED:
      DEBUG_PRINTLN("未配网");
      break;
    case STATUS_WIFI_CONNECTING:
      DEBUG_PRINTLN("WiFi连接中");
      break;
    case STATUS_MQTT_CONNECTING:
      DEBUG_PRINTLN("MQTT连接中");
      break;
    case STATUS_READY:
      DEBUG_PRINTLN("就绪");
      break;
    }
  }
}

void LEDIndicator::blinkIR() {
  setIRLED(true);
  irBlinkStart = millis();
}

void LEDIndicator::setSystemLED(bool state) {
  digitalWrite(PIN_LED_SYS, state ? HIGH : LOW);
}

void LEDIndicator::setIRLED(bool state) {
  digitalWrite(PIN_LED_IR, state ? HIGH : LOW);
}
