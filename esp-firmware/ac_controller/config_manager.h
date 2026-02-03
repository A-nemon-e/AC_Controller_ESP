/*
 * 配置管理模块
 *
 * 功能：
 * - 从EEPROM加载配置
 * - 保存配置到EEPROM
 * - 支持MQTT动态配置更新
 * - 生成默认配置（使用MAC地址）
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

// 配置结构体
struct DeviceConfig {
  char mqttServer[64];     // MQTT服务器地址
  uint16_t mqttPort;       // MQTT端口
  char mqttUser[32];       // MQTT用户名
  char mqttPassword[64];   // MQTT密码
  char deviceUUID[32];     // 设备UUID
  uint32_t userId;         // 用户ID
  uint32_t sensorInterval; // 传感器上报间隔
  uint32_t ghostWindow;    // Ghost检测窗口

  // ✅ 品牌协议配置
  char brand[16]; // 品牌："GREE", "MIDEA", "DAIKIN" 等
  uint8_t model;  // 型号代码 (0-255)

  uint8_t checksum; // 校验和
};

class ConfigManager {
public:
  // 初始化配置管理器
  static void init();

  // 加载配置（从EEPROM或生成默认配置）
  static bool load();

  // 保存配置到EEPROM
  static bool save();

  // 重置为默认配置
  static void resetToDefault();

  // 更新配置（从JSON字符串）
  static bool updateFromJSON(const char *json);

  // 获取配置
  static DeviceConfig &getConfig();

  // 打印配置信息
  static void printConfig();

private:
  static DeviceConfig config;
  static bool loaded;

  // 计算校验和
  static uint8_t calculateChecksum();

  // 验证校验和
  static bool verifyChecksum();

  // 生成基于MAC的默认UUID
  static String generateUUID();

  // EEPROM配置存储地址
  static const uint16_t EEPROM_CONFIG_ADDR = 256;
};

#endif // CONFIG_MANAGER_H
