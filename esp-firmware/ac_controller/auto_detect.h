/*
 * 自动协议检测模块
 *
 * 功能：
 * - 接收红外信号自动识别品牌协议
 * - 提取品牌和型号信息
 * - 解析空调状态参数
 * - 支持降级到Raw模式
 */

#ifndef AUTO_DETECT_H
#define AUTO_DETECT_H

#include "config.h"
#include <Arduino.h>
#include <IRac.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <ir_Daikin.h>
#include <ir_Fujitsu.h>
#include <ir_Gree.h>
#include <ir_Haier.h>
#include <ir_Midea.h>


// 检测结果结构
struct DetectionResult {
  bool success;       // 是否成功识别
  String protocol;    // 协议名称 "GREE", "MIDEA", "UNKNOWN"
  int model;          // 型号代码 (0-255)
  String description; // 人类可读描述
  String rawData;     // 原始数据（用于Raw模式）

  // 解析出的空调状态（如果是AC协议）
  bool isAC; // 是否是空调协议
  bool power;
  String mode;  // "cool", "heat", "dry", "fan", "auto"
  uint8_t temp; // 温度 (16-30)
  uint8_t fan;  // 风速 (0-3)
  bool swingV;  // 垂直摆风
  bool swingH;  // 水平摆风
};

class AutoDetect {
public:
  // 启动自动检测模式
  static void start();

  // 停止检测
  static void stop();

  // 处理接收到的红外信号（核心函数）
  static DetectionResult analyze(decode_results *results);

  // 是否正在检测
  static bool isDetecting();

  // 获取检测状态信息
  static String getStatus();

private:
  static bool detecting;
  static unsigned long startTime;
  static const uint32_t DETECT_TIMEOUT = 30000; // 30秒超时

  // 从state数组提取型号（品牌特定）
  static int extractModel(decode_type_t protocol, uint8_t *state);

  // 解析AC状态
  static void parseACState(decode_results *results, DetectionResult &result);

  // 获取协议名称字符串
  static String getProtocolName(decode_type_t type);

  // 判断是否是AC协议
  static bool isACProtocol(decode_type_t type);
};

#endif // AUTO_DETECT_H
