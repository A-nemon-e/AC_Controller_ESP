/*
 * WiFi管理模块
 *
 * 功能：
 * - SmartConfig配网
 * - WiFi连接和重连
 * - EEPROM存储WiFi凭证
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include "led_indicator.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>


class WiFiManager {
public:
  // 初始化并连接WiFi
  static void connect();

  // 维护WiFi连接（在loop中调用）
  static void maintain();

  // 检查是否已连接
  static bool isConnected();

  // 启动SmartConfig配网
  static void startSmartConfig();

  // 获取IP地址
  static String getIPAddress();

  // 获取MAC地址
  static String getMACAddress();

private:
  static bool configured;
  static unsigned long lastReconnectAttempt;

  // 从EEPROM读取WiFi凭证
  static bool loadCredentials(String &ssid, String &password);

  // 保存WiFi凭证到EEPROM
  static void saveCredentials(const String &ssid, const String &password);

  // 尝试连接WiFi
  static bool attemptConnection(const String &ssid, const String &password);
};

#endif // WIFI_MANAGER_H
