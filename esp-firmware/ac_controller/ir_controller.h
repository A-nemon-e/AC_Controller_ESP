/*
 * 红外控制器模块
 *
 * 功能：
 * - 发送红外信号（支持品牌协议和原始数据）
 * - 接收红外信号
 * - 触发Ghost检测
 */

#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#include "config.h"
#include "led_indicator.h"
#include <Arduino.h>
#include <IRac.h> // ✅ 新增：品牌协议统一接口
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>

class IRController {
public:
  // 初始化红外模块
  static void init();

  // 发送原始时序数据（学习模式核心）
  static bool sendRaw(const char *rawDataStr);
  static bool sendRaw(uint16_t *rawData, uint16_t length);

  // ✅ 新增：发送品牌协议
  static bool
  sendBrand(const char *brand, // 品牌："GREE", "MIDEA", "DAIKIN" 等
            int model,         // 型号代码
            bool power,        // 开关
            const char *mode,  // 模式："cool", "heat", "dry", "fan", "auto"
            uint8_t temp,      // 温度 (16-30)
            uint8_t fanSpeed,  // 风速 (0=auto, 1=low, 2=mid, 3=high)
            bool swingV,       // 垂直摆风
            bool swingH        // 水平摆风
  );

  // 处理红外接收（在loop中调用）
  static void handleReceive();

  // 获取最后接收到的原始数据
  static String getLastRawData();

  // 设置接收回调
  static void setReceiveCallback(void (*callback)(decode_results *));

  // ✅ 新增：获取支持的品牌列表JSON
  static String getSupportedBrandsJSON();

private:
  static IRsend irsend;
  static IRrecv irrecv;
  static IRac ac; // ✅ 新增：统一AC控制器
  static decode_results results;
  static void (*receiveCallback)(decode_results *);

  // 解析原始数据字符串（"9000,4500,560,..."）
  static uint16_t parseRawString(const char *str, uint16_t *buffer,
                                 uint16_t maxLen);

  // 将decode_results转换为字符串
  static String resultsToRawString(decode_results *results);

  // ✅ 新增：品牌字符串转协议类型
  static decode_type_t stringToProtocol(const char *brand);
};

#endif // IR_CONTROLLER_H
