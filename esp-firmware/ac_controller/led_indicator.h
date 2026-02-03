/*
 * LED指示器模块
 *
 * 功能：
 * - 系统状态指示（GPIO15）：WiFi、MQTT连接状态
 * - 红外接收指示（GPIO16）：红外信号接收闪烁
 */

#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "config.h"
#include <Arduino.h>


// 系统状态枚举
enum SystemStatus {
  STATUS_UNCONFIGURED,    // 未配网（快闪200ms）
  STATUS_WIFI_CONNECTING, // WiFi连接中（慢闪1000ms）
  STATUS_MQTT_CONNECTING, // MQTT连接中（快闪500ms）
  STATUS_READY            // 正常运行（常亮）
};

class LEDIndicator {
public:
  // 初始化LED引脚
  static void init();

  // 更新LED状态（在loop中调用）
  static void update();

  // 设置系统状态
  static void setStatus(SystemStatus status);

  // 红外接收指示（闪烁100ms）
  static void blinkIR();

private:
  static SystemStatus currentStatus;
  static unsigned long lastBlinkTime;
  static bool ledState;
  static unsigned long irBlinkStart;

  // 辅助函数
  static void setSystemLED(bool state);
  static void setIRLED(bool state);
};

#endif // LED_INDICATOR_H
