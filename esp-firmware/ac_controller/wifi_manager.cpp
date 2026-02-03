/*
 * WiFi管理模块 - 实现
 */

#include "wifi_manager.h"

// 静态成员初始化
bool WiFiManager::configured = false;
unsigned long WiFiManager::lastReconnectAttempt = 0;

void WiFiManager::connect() {
  DEBUG_PRINTLN("\n[WiFi] 初始化WiFi模块");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

// ⚠️ 测试环境：优先使用硬编码WiFi凭证
#ifdef WIFI_SSID
  DEBUG_PRINTLN("[WiFi] 使用硬编码WiFi凭证");
  DEBUG_PRINTF("[WiFi] SSID: %s\n", WIFI_SSID);
  configured = attemptConnection(WIFI_SSID, WIFI_PASSWORD);

  if (configured) {
    // 连接成功，保存到EEPROM以便下次使用
    saveCredentials(WIFI_SSID, WIFI_PASSWORD);
    return;
  } else {
    DEBUG_PRINTLN("[WiFi] 硬编码凭证连接失败，尝试其他方式...");
  }
#endif

  // 尝试从EEPROM读取凭证
  String ssid, password;
  if (loadCredentials(ssid, password)) {
    DEBUG_PRINTLN("[WiFi] 找到保存的凭证，尝试连接...");
    configured = attemptConnection(ssid, password);
  }

  // 如果没有凭证或连接失败，启动SmartConfig
  if (!configured) {
    DEBUG_PRINTLN("[WiFi] 未配置或连接失败，启动SmartConfig");
    startSmartConfig();
  }
}

void WiFiManager::maintain() {
  // 如果WiFi断开，尝试重连
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();

    // 避免频繁重连
    if (now - lastReconnectAttempt > WIFI_RECONNECT_DELAY) {
      lastReconnectAttempt = now;

      DEBUG_PRINTLN("[WiFi] 连接丢失，尝试重连...");
      LEDIndicator::setStatus(STATUS_WIFI_CONNECTING);

      String ssid, password;
      if (loadCredentials(ssid, password)) {
        WiFi.begin(ssid.c_str(), password.c_str());
      }
    }
  } else if (!configured) {
    // 第一次连接成功
    configured = true;
    DEBUG_PRINT("[WiFi] ✅ 连接成功！IP: ");
    DEBUG_PRINTLN(WiFi.localIP().toString());
  }
}

bool WiFiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }

void WiFiManager::startSmartConfig() {
  LEDIndicator::setStatus(STATUS_UNCONFIGURED);

  DEBUG_PRINTLN("[WiFi] SmartConfig模式启动");
  DEBUG_PRINTLN("[WiFi] 请使用手机APP进行配网");

  WiFi.beginSmartConfig();

  // 等待SmartConfig完成（最多2分钟）
  unsigned long startTime = millis();
  while (!WiFi.smartConfigDone()) {
    delay(500);
    DEBUG_PRINT(".");

    // 超时检查
    if (millis() - startTime > 120000) {
      DEBUG_PRINTLN("\n[WiFi] ❌ SmartConfig超时");
      return;
    }
  }

  DEBUG_PRINTLN("\n[WiFi] SmartConfig完成");

  // 等待IP地址获取
  LEDIndicator::setStatus(STATUS_WIFI_CONNECTING);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN();
  DEBUG_PRINT("[WiFi] ✅ 连接成功！IP: ");
  DEBUG_PRINTLN(WiFi.localIP().toString());

  // 保存凭证到EEPROM
  saveCredentials(WiFi.SSID(), WiFi.psk());
  configured = true;
}

String WiFiManager::getIPAddress() { return WiFi.localIP().toString(); }

String WiFiManager::getMACAddress() { return WiFi.macAddress(); }

bool WiFiManager::loadCredentials(String &ssid, String &password) {
  EEPROM.begin(EEPROM_SIZE);

  // 检查配置标志
  if (EEPROM.read(EEPROM_CONFIG_FLAG) != 0x55) {
    EEPROM.end();
    return false;
  }

  // 读取SSID
  char ssidBuf[33] = {0};
  for (int i = 0; i < 32; i++) {
    ssidBuf[i] = EEPROM.read(EEPROM_WIFI_SSID + i);
    if (ssidBuf[i] == 0)
      break;
  }
  ssid = String(ssidBuf);

  // 读取密码
  char passBuf[65] = {0};
  for (int i = 0; i < 64; i++) {
    passBuf[i] = EEPROM.read(EEPROM_WIFI_PASS + i);
    if (passBuf[i] == 0)
      break;
  }
  password = String(passBuf);

  EEPROM.end();

  return ssid.length() > 0;
}

void WiFiManager::saveCredentials(const String &ssid, const String &password) {
  EEPROM.begin(EEPROM_SIZE);

  // 写入SSID
  for (int i = 0; i < 32; i++) {
    if (i < ssid.length()) {
      EEPROM.write(EEPROM_WIFI_SSID + i, ssid[i]);
    } else {
      EEPROM.write(EEPROM_WIFI_SSID + i, 0);
    }
  }

  // 写入密码
  for (int i = 0; i < 64; i++) {
    if (i < password.length()) {
      EEPROM.write(EEPROM_WIFI_PASS + i, password[i]);
    } else {
      EEPROM.write(EEPROM_WIFI_PASS + i, 0);
    }
  }

  // 设置配置标志
  EEPROM.write(EEPROM_CONFIG_FLAG, 0x55);

  EEPROM.commit();
  EEPROM.end();

  DEBUG_PRINTLN("[WiFi] 凭证已保存到EEPROM");
}

bool WiFiManager::attemptConnection(const String &ssid,
                                    const String &password) {
  DEBUG_PRINTF("[WiFi] 尝试连接: %s\n", ssid.c_str());
  LEDIndicator::setStatus(STATUS_WIFI_CONNECTING);

  WiFi.begin(ssid.c_str(), password.c_str());

  // 等待连接（最多20秒）
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");

    if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
      DEBUG_PRINTLN("\n[WiFi] ❌ 连接超时");
      return false;
    }
  }

  DEBUG_PRINTLN();
  return true;
}
