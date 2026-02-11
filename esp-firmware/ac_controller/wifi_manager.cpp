/*
 * WiFi管理模块 - 实现
 */

#include "wifi_manager.h"

// 静态成员初始化
bool WiFiManager::configured = false;
unsigned long WiFiManager::lastReconnectAttempt = 0;
ESP8266WebServer WiFiManager::server(80); // ✅ Web服务器监听80端口
DNSServer WiFiManager::dnsServer;         // ✅ DNS服务器实例

void WiFiManager::connect() {
  DEBUG_PRINTLN("\n[WiFi] 初始化WiFi模块 (3层连接策略)");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  // ===== Layer 1: 硬编码凭证 (优先级最高) =====
#ifdef WIFI_SSID
  DEBUG_PRINTLN("[WiFi] [L1] 尝试硬编码凭证...");
  DEBUG_PRINTF("[WiFi] SSID: %s\n", WIFI_SSID);

  // 尝试2次，超时45s -> 每次约22500ms
  for (int i = 0; i < 2; i++) {
    if (attemptConnection(WIFI_SSID, WIFI_PASSWORD, 22500)) {
      DEBUG_PRINTLN("[WiFi] [L1] ✅ 连接成功");
      configured = true;
      // 保存到EEPROM作为Layer 2备份
      saveCredentials(WIFI_SSID, WIFI_PASSWORD);
      return;
    }
    if (i == 0)
      DEBUG_PRINTLN("[WiFi] [L1] 重试...");
  }
  DEBUG_PRINTLN("[WiFi] [L1] ❌ 连接失败");
#else
  DEBUG_PRINTLN("[WiFi] [L1] 未定义硬编码凭证，跳过");
#endif

  // ===== Layer 2: EEPROM凭证 (无Checksum) =====
  String ssid, password;
  DEBUG_PRINTLN("[WiFi] [L2] 尝试读取EEPROM凭证...");

  if (loadCredentials(ssid, password)) {
    DEBUG_PRINTF("[WiFi] 读取到SSID: %s\n", ssid.c_str());

    // 尝试2次，超时30s -> 每次约15000ms
    for (int i = 0; i < 2; i++) {
      if (attemptConnection(ssid, password, 15000)) {
        DEBUG_PRINTLN("[WiFi] [L2] ✅ 连接成功");
        configured = true;
        return;
      }
      if (i == 0)
        DEBUG_PRINTLN("[WiFi] [L2] 重试...");
    }
    DEBUG_PRINTLN("[WiFi] [L2] ❌ 连接失败");
  } else {
    DEBUG_PRINTLN("[WiFi] [L2] EEPROM中无有效凭证");
  }

  // ===== Layer 3: AP模式 + Web配置 =====
  DEBUG_PRINTLN("[WiFi] [L3] 启动AP配网模式...");
  startAPMode();
}

void WiFiManager::maintain() {
  // 如果WiFi断开，尝试重连
  if (WiFi.status() != WL_CONNECTED) {
    // 如果处于AP模式，处理Web请求
    // 注意：startAPMode是阻塞的直到重启，所以这里 maintain 主要用于 STA
    // 模式下的掉线重连 如果 connect() 在 L3 阶段不阻塞（本实现中 L3
    // 是阻塞的），这里才需要处理

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

void WiFiManager::startAPMode() {
  // 切换到 AP+STA 模式 (或者仅 AP)
  WiFi.mode(WIFI_AP);

  String apName = "ESP_Setup_" + String(ESP.getChipId(), HEX);
  DEBUG_PRINTF("[WiFi] AP名称: %s\n", apName.c_str());
  DEBUG_PRINTLN("[WiFi] 请连接热点并访问 192.168.4.1 配置WiFi");

  WiFi.softAP(apName.c_str());

  // ✅ 启动DNS服务器，将所有请求重定向到本机IP
  // 53是DNS端口，"*"代表匹配所有域名
  dnsServer.start(53, "*", WiFi.softAPIP());
  DEBUG_PRINTLN("[WiFi] DNS服务器已启动 (Captive Portal)");

  // 启动Web服务器
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  // ✅ 捕获所有其他请求并重定向到根目录 (Android/iOS Captive Portal检测)
  server.onNotFound(handleRoot);
  server.begin();

  DEBUG_PRINTLN("[WiFi] Web服务器已启动");
  LEDIndicator::setStatus(STATUS_UNCONFIGURED); // 慢闪提示

  // 阻塞循环，处理Web请求
  while (true) {
    dnsServer.processNextRequest(); // ✅ 处理DNS请求
    server.handleClient();
    LEDIndicator::update();
    delay(10);
  }
}

void WiFiManager::handleRoot() {
  String html =
      "<html><head><meta name='viewport' content='width=device-width, "
      "initial-scale=1, user-scalable=no'>";
  html += "<title>WiFi Configuration</title>";
  html += "<style>body{font-family:Arial;padding:20px;max-width:400px;margin:0 "
          "auto;}";
  html += "input{width:100%;padding:10px;margin:10px 0;box-sizing:border-box;}";
  html += "button{width:100%;padding:10px;background:#007bff;color:white;"
          "border:none;cursor:pointer;}";
  html += ".note{color:orange;font-size:14px;}</style></head><body>";
  html += "<h2>WiFi Configuration</h2>";
  html += "<p class='note'>Note: Only 2.4GHz WiFi is supported.</p>";
  html += "<form action='/save' method='POST'>";
  html +=
      "<input type='text' name='ssid' placeholder='WiFi Name (SSID)' required>";
  html +=
      "<input type='password' name='password' placeholder='Password' required>";
  html += "<button type='submit'>Save & Restart</button>";
  html += "</form></body></html>";

  server.send(200, "text/html", html);
}

void WiFiManager::handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  if (ssid.length() > 0) {
    DEBUG_PRINTLN("[WiFi] 收到Web配置:");
    DEBUG_PRINTF("SSID: %s\n", ssid.c_str());

    saveCredentials(ssid, password);

    String html = "<html><body><h2>Saved!</h2><p>Device is "
                  "restarting...</p></body></html>";
    server.send(200, "text/html", html);

    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "SSID cannot be empty");
  }
}

String WiFiManager::getIPAddress() { return WiFi.localIP().toString(); }

String WiFiManager::getMACAddress() { return WiFi.macAddress(); }

bool WiFiManager::loadCredentials(String &ssid, String &password) {
  EEPROM.begin(EEPROM_SIZE);

  // ⚠️ 移除 EEPROM_CONFIG_FLAG 检查 (Layer 2 无 Checksum)
  // 直接尝试读取 SSID
  // if (EEPROM.read(EEPROM_CONFIG_FLAG) != 0x55) ...

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

  // 简单验证：SSID不为空且字符合法
  if (ssid.length() == 0 || ssid.length() > 32)
    return false;

  // 检查是否全由 0xFF 组成（即空EEPROM）
  if (ssid[0] == (char)0xFF)
    return false;

  return true;
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

  // ⚠️ 移除 EEPROM_CONFIG_FLAG 写入
  // EEPROM.write(EEPROM_CONFIG_FLAG, 0x55);

  EEPROM.commit();
  EEPROM.end();

  DEBUG_PRINTLN("[WiFi] 凭证已保存到EEPROM");
}

bool WiFiManager::attemptConnection(const String &ssid, const String &password,
                                    unsigned long timeout) {
  DEBUG_PRINTF("[WiFi] 尝试连接: %s (超时: %lu ms)\n", ssid.c_str(), timeout);
  LEDIndicator::setStatus(STATUS_WIFI_CONNECTING);

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");

    if (millis() - startTime > timeout) {
      DEBUG_PRINTLN("\n[WiFi] ❌ 连接超时");
      return false;
    }
  }

  DEBUG_PRINTLN();
  return true;
}
