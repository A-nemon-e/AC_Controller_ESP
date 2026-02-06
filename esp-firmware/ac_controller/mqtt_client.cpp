/*
 * MQTT客户端模块 - 实现
 */

#include "mqtt_client.h"
#include "config_manager.h"

// 静态成员初始化
WiFiClient MQTTClient::wifiClient;
PubSubClient MQTTClient::mqttClient(wifiClient);
bool MQTTClient::connected = false;
unsigned long MQTTClient::lastReconnectAttempt = 0;
void (*MQTTClient::externalCallback)(char *, uint8_t *, unsigned int) = nullptr;

// 故障回退机制变量
uint8_t MQTTClient::eepromFailCount = 0;
bool MQTTClient::useDefaultCredentials = false;

void MQTTClient::connect() {
  DEBUG_PRINTLN("[MQTT] 初始化MQTT客户端");

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(messageCallback);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);

  // 首次连接
  reconnect();
}

void MQTTClient::loop() {
  if (!mqttClient.connected()) {
    unsigned long now = millis();

    // 避免频繁重连
    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY) {
      lastReconnectAttempt = now;

      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
  }
}

bool MQTTClient::isConnected() { return mqttClient.connected(); }

bool MQTTClient::publish(const char *topic, const char *payload) {
  return publish(topic, payload, false);
}

bool MQTTClient::publish(const char *topic, const char *payload,
                         bool retained) {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN("[MQTT] ❌ 未连接，无法发布消息");
    return false;
  }

  DEBUG_PRINTF("[MQTT] 发布: %s\n", topic);
  DEBUG_PRINTF("[MQTT] 内容: %s\n", payload);

  return mqttClient.publish(topic, payload, retained);
}

bool MQTTClient::subscribe(const char *topic) {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN("[MQTT] ❌ 未连接，无法订阅");
    return false;
  }

  DEBUG_PRINTF("[MQTT] 订阅: %s\n", topic);
  return mqttClient.subscribe(topic);
}

void MQTTClient::setCallback(void (*callback)(char *, uint8_t *,
                                              unsigned int)) {
  externalCallback = callback;
}

String MQTTClient::getTopic(const char *suffix) {
  // 生成topic: ac/user_{userId}/dev_{uuid}/{suffix}
  // ✅ 修复：使用 EEPROM 配置中的 userId，而不是硬编码的 USER_ID
  DeviceConfig &cfg = ConfigManager::getConfig();

  char topic[128];
  snprintf(topic, sizeof(topic), "ac/user_%u/dev_%s/%s",
           cfg.userId,     // ← 使用 EEPROM 中的 userId
           cfg.deviceUUID, // ← 使用 EEPROM 中的 UUID
           suffix);
  return String(topic);
}

void MQTTClient::messageCallback(char *topic, uint8_t *payload,
                                 unsigned int length) {
  DEBUG_PRINTF("[MQTT] 收到消息: %s\n", topic);
  DEBUG_PRINT("[MQTT] 内容: ");
  for (unsigned int i = 0; i < length; i++) {
    DEBUG_PRINT((char)payload[i]);
  }
  DEBUG_PRINTLN();

  // 调用外部回调函数
  if (externalCallback != nullptr) {
    externalCallback(topic, payload, length);
  }
}

bool MQTTClient::reconnect() {
  if (mqttClient.connected()) {
    return true;
  }

  DEBUG_PRINTLN("[MQTT] 尝试连接MQTT服务器...");
  LEDIndicator::setStatus(STATUS_MQTT_CONNECTING);

  // 生成客户端ID（使用MAC地址）
  String clientId = "ESP_AC_";
  clientId += WiFi.macAddress();
  clientId.replace(":", "");

  // 从配置获取凭证
  DeviceConfig &cfg = ConfigManager::getConfig();

  // 配置有效性检查：防止使用 EEPROM 中的垃圾数据
  // 检查字符串是否为空或包含不可打印字符（如 0xFF）
  auto isValidString = [](const char *str, size_t maxLen) -> bool {
    if (str[0] == '\0' || str[0] == (char)0xFF)
      return false;
    // 检查是否包含可打印字符
    for (size_t i = 0; i < maxLen && str[i] != '\0'; i++) {
      if (str[i] < 32 || str[i] > 126)
        return false; // 不是 ASCII 可打印字符
    }
    return true;
  };

  // 智能回退机制：EEPROM 配置失败多次后强制使用默认值
  bool eepromConfigAvailable =
      isValidString(cfg.mqttUser, sizeof(cfg.mqttUser)) &&
      isValidString(cfg.mqttPassword, sizeof(cfg.mqttPassword));

  // 如果 EEPROM 配置已失败超过阈值，强制使用默认值
  if (eepromConfigAvailable && eepromFailCount >= MAX_EEPROM_FAIL) {
    DEBUG_PRINTF("[MQTT] ⚠️ EEPROM配置已失败%d次，强制回退到默认值\n",
                 eepromFailCount);
    useDefaultCredentials = true;
    eepromConfigAvailable = false;
  }

  const char *mqttUser = (eepromConfigAvailable && !useDefaultCredentials)
                             ? cfg.mqttUser
                             : MQTT_USER;
  const char *mqttPassword = (eepromConfigAvailable && !useDefaultCredentials)
                                 ? cfg.mqttPassword
                                 : MQTT_PASSWORD;

  // 调试：输出 MQTT 连接信息
  DEBUG_PRINTF("[MQTT] 服务器: %s:%d\n", MQTT_SERVER, MQTT_PORT);
  DEBUG_PRINTF("[MQTT] 客户端ID: %s\n", clientId.c_str());
  DEBUG_PRINTF("[MQTT] 用户名: %s %s\n", mqttUser,
               (eepromConfigAvailable && !useDefaultCredentials)
                   ? "(来自EEPROM)"
                   : "(使用默认值)");
  DEBUG_PRINTF("[MQTT] 密码长度: %d %s\n", strlen(mqttPassword),
               (eepromConfigAvailable && !useDefaultCredentials)
                   ? "(来自EEPROM)"
                   : "(使用默认值)");
  if (useDefaultCredentials) {
    DEBUG_PRINTF("[MQTT] ⚠️ 已回退到默认值 (失败次数: %d/%d)\n", eepromFailCount,
                 MAX_EEPROM_FAIL);
  }

  // LWT 配置
  String availTopic = getTopic("availability");
  const char *willTopic = availTopic.c_str(); // ac/user_x/dev_uuid/availability
  const char *willMsg = "offline";
  int willQoS = 1;
  bool willRetain = true;

  DEBUG_PRINTF("[MQTT] LWT Topic: %s\n", willTopic);

  // 尝试连接（使用回退后的配置）
  // connect(clientId, username, password, willTopic, willQoS, willRetain,
  // willMessage)
  if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword, willTopic,
                         willQoS, willRetain, willMsg)) {
    DEBUG_PRINTLN("[MQTT] ✅ 连接成功");
    connected = true;
    LEDIndicator::setStatus(STATUS_READY);

    // 连接成功后重置失败计数
    eepromFailCount = 0;
    useDefaultCredentials = false;

    // 订阅控制命令topic
    String cmdTopic = getTopic("cmd");
    String learnTopic = getTopic("learn/start");
    String configTopic = getTopic("config");
    String configUpdateTopic = getTopic("config/update");
    String autoDetectTopic = getTopic("auto_detect"); // ✅ 新增
    String brandsGetTopic = getTopic("brands/get");   // ✅ 新增
    String sceneSaveTopic = getTopic("scene/save");   // ✅ 新增

    subscribe(cmdTopic.c_str());
    subscribe(learnTopic.c_str());
    subscribe(configTopic.c_str());
    subscribe(configUpdateTopic.c_str());
    subscribe(autoDetectTopic.c_str()); // ✅
    subscribe(brandsGetTopic.c_str());  // ✅
    subscribe(sceneSaveTopic.c_str());  // ✅

    // 发布上线消息 (至 availability topic)
    publish(willTopic, "online", true); // Retained = true

    return true;
  } else {
    DEBUG_PRINT("[MQTT] ❌ 连接失败，错误码: ");
    DEBUG_PRINTLN(mqttClient.state());
    connected = false;

    // 如果使用的是 EEPROM 配置而非默认值，增加失败计数
    if (eepromConfigAvailable && !useDefaultCredentials) {
      eepromFailCount++;
      DEBUG_PRINTF("[MQTT] EEPROM配置失败计数: %d/%d\n", eepromFailCount,
                   MAX_EEPROM_FAIL);
    }

    return false;
  }
}

// ✅ 新增：重新订阅topic（设备绑定后调用）
void MQTTClient::resubscribe() {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN("[MQTT] ❌ 未连接，无法重新订阅");
    return;
  }

  DEBUG_PRINTLN("[MQTT] 🔄 重新订阅topic（设备绑定后更新）");

  // 订阅新的topic（基于更新后的userId）
  String cmdTopic = getTopic("cmd");
  String learnTopic = getTopic("learn/start");
  String configTopic = getTopic("config");
  String configUpdateTopic = getTopic("config/update");
  String autoDetectTopic = getTopic("auto_detect"); // ✅ 新增
  String brandsGetTopic = getTopic("brands/get");   // ✅ 新增
  String sceneSaveTopic = getTopic("scene/save");   // ✅ 新增

  subscribe(cmdTopic.c_str());
  subscribe(learnTopic.c_str());
  subscribe(configTopic.c_str());
  subscribe(configUpdateTopic.c_str());
  subscribe(autoDetectTopic.c_str()); // ✅
  subscribe(brandsGetTopic.c_str());  // ✅
  subscribe(sceneSaveTopic.c_str());  // ✅

  DEBUG_PRINTLN("[MQTT] ✅ 重新订阅完成");
}
