/*
 * 红外学习模块
 *
 * 功能：
 * - 进入学习模式
 * - 捕获遥控器原始数据
 * - 发布学习结果到MQTT
 * - 超时处理
 */

#ifndef IR_LEARNING_H
#define IR_LEARNING_H

#include "config.h"
#include "ir_controller.h"
#include <Arduino.h>


class IRLearning {
public:
  // 启动学习模式
  static void start(const char *key);

  // 停止学习模式
  static void stop();

  // 检查是否在学习模式
  static bool isLearning();

  // 更新学习状态（在loop中调用）
  static void update();

  // 处理接收到的红外数据（由IRController回调）
  static void onIRReceived(decode_results *results);

private:
  static bool learning;
  static char learningKey[32];
  static unsigned long learningStartTime;

  // 发布学习结果
  static void publishResult(const char *rawData);

  // 发布学习失败
  static void publishTimeout();
};

#endif // IR_LEARNING_H
