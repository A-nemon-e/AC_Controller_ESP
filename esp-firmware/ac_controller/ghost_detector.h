/*
 * Ghost检测器模块
 *
 * 功能：
 * - 监测麦克风信号
 * - 对比红外接收时间
 * - 检测手动遥控器操作（Ghost）
 * - 发布Ghost事件
 */

#ifndef GHOST_DETECTOR_H
#define GHOST_DETECTOR_H

#include "config.h"
#include <Arduino.h>


// 麦克风配置
struct MicConfig {
  bool enabled;
  uint8_t sensitivity;
  uint16_t beepDurationMs;
  String beepType; // "short", "long", "double"
};

class GhostDetector {
public:
  // 初始化Ghost检测器
  static void init();

  // 更新检测状态（在loop中调用）
  static void update();

  // 红外接收回调
  static void onIRReceived();

  // 麦克风触发回调
  static void onMicTriggered();

  // 更新麦克风配置
  static void updateMicConfig(bool enabled, uint8_t sensitivity,
                              uint16_t beepDuration, const char *beepType);

  // 检查是否启用
  static bool isEnabled();

private:
  static MicConfig micConfig;
  static unsigned long lastIRTime;
  static unsigned long lastMicTime;
  static bool lastMicState;

  // 发布Ghost事件
  static void publishGhostEvent();
};

#endif // GHOST_DETECTOR_H
