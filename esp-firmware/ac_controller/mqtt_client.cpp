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
  char topic[128];
  snprintf(topic, sizeof(topic), "ac/user_%d/dev_%s/%s", USER_ID, DEVICE_UUID,
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

  // 尝试连接
  if (mqttClient.connect(clientId.c_str(), cfg.mqttUser, cfg.mqttPassword)) {
    DEBUG_PRINTLN("[MQTT] ✅ 连接成功");
    connected = true;
    LEDIndicator::setStatus(STATUS_READY);

    // 订阅控制命令topic
    String cmdTopic = getTopic("cmd");
    String learnTopic = getTopic("learn/start");
    String configTopic = getTopic("config");

    subscribe(cmdTopic.c_str());
    subscribe(learnTopic.c_str());
    subscribe(configTopic.c_str());

    // 发布上线消息
    String statusTopic = getTopic("status");
    publish(statusTopic.c_str(), "{\"online\":true}", true);

    return true;
  } else {
    DEBUG_PRINT("[MQTT] ❌ 连接失败，错误码: ");
    DEBUG_PRINTLN(mqttClient.state());
    connected = false;
    return false;
  }
}
