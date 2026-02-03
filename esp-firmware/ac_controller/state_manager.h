/*
 * 空调状态管理器
 *
 * 功能：
 * - 维护空调当前状态
 * - 状态变更历史
 * - EEPROM持久化（可选）
 * - 状态发布
 */

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "config.h"
#include <Arduino.h>


// 空调状态结构
struct AirConditionerState {
  bool power;
  String mode;   // "cool", "heat", "fan", "dry", "auto"
  uint8_t temp;  // 16-30
  uint8_t fan;   // 0-3 (auto, low, mid, high)
  bool swingV;   // 垂直扫风
  bool swingH;   // 水平扫风
  String source; // "api", "ir_recv", "manual"
  unsigned long lastUpdate;
};

class StateManager {
public:
  // 初始化状态管理器
  static void init();

  // 设置状态
  static void setState(bool power, const char *mode, uint8_t temp, uint8_t fan,
                       bool swingV, bool swingH, const char *source);

  // 从JSON更新状态
  static bool updateFromJSON(const char *json);

  // 获取当前状态
  static AirConditionerState &getState();

  // 发布状态到MQTT
  static void publishState();

  // 保存到EEPROM（可选）
  static void save();

  // 从EEPROM加载（可选）
  static bool load();

private:
  static AirConditionerState currentState;
  static bool stateChanged;
};

#endif // STATE_MANAGER_H
