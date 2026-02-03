/*
 * MQTT客户端模块
 *
 * 功能：
 * - 连接MQTT Broker
 * - 订阅控制命令topic
 * - 发布状态和事件
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "config.h"
#include "led_indicator.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


class MQTTClient {
public:
  // 初始化并连接MQTT
  static void connect();

  // 维护MQTT连接（在loop中调用）
  static void loop();

  // 检查是否已连接
  static bool isConnected();

  // 发布消息
  static bool publish(const char *topic, const char *payload);
  static bool publish(const char *topic, const char *payload, bool retained);

  // 订阅topic
  static bool subscribe(const char *topic);

  // 设置消息回调函数
  static void setCallback(void (*callback)(char *, uint8_t *, unsigned int));

  // 生成topic（辅助函数）
  static String getTopic(const char *suffix);

private:
  static WiFiClient wifiClient;
  static PubSubClient mqttClient;
  static bool connected;
  static unsigned long lastReconnectAttempt;

  // MQTT消息回调（内部）
  static void messageCallback(char *topic, uint8_t *payload,
                              unsigned int length);

  // 外部回调函数指针
  static void (*externalCallback)(char *, uint8_t *, unsigned int);

  // 重连
  static bool reconnect();
};

#endif // MQTT_CLIENT_H
