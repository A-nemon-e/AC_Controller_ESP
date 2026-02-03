/*
 * 红外学习模块 - 实现
 */

#include "ir_learning.h"
#include "led_indicator.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>


// 静态成员初始化
bool IRLearning::learning = false;
char IRLearning::learningKey[32] = {0};
unsigned long IRLearning::learningStartTime = 0;

void IRLearning::start(const char *key) {
  DEBUG_PRINTF("[学习] 启动学习模式: %s\n", key);

  learning = true;
  strncpy(learningKey, key, sizeof(learningKey) - 1);
  learningStartTime = millis();

  // LED指示进入学习模式（快闪）
  LEDIndicator::setStatus(STATUS_MQTT_CONNECTING); // 复用快闪状态

  DEBUG_PRINTLN("[学习] ⏰ 等待红外信号（30秒）");
}

void IRLearning::stop() {
  if (learning) {
    DEBUG_PRINTLN("[学习] 退出学习模式");
    learning = false;
    LEDIndicator::setStatus(STATUS_READY);
  }
}

bool IRLearning::isLearning() { return learning; }

void IRLearning::update() {
  if (!learning)
    return;

  // 检查超时（30秒）
  if (millis() - learningStartTime > IR_LEARNING_TIMEOUT) {
    DEBUG_PRINTLN("[学习] ❌ 学习超时");
    publishTimeout();
    stop();
  }
}

void IRLearning::onIRReceived(decode_results *results) {
  if (!learning)
    return;

  DEBUG_PRINTLN("[学习] ✅ 捕获到红外信号");

  // 获取原始数据
  String rawData = IRController::getLastRawData();

  DEBUG_PRINTF("[学习] 原始数据长度: %d\n", rawData.length());
  DEBUG_PRINTF("[学习] 数据: %s\n", rawData.c_str());

  // 发布学习结果
  publishResult(rawData.c_str());

  // 退出学习模式
  stop();
}

void IRLearning::publishResult(const char *rawData) {
  if (!MQTTClient::isConnected()) {
    DEBUG_PRINTLN("[学习] ⚠️  MQTT未连接，无法发布结果");
    return;
  }

  // 构建JSON消息
  StaticJsonDocument<1024> doc;
  doc["key"] = learningKey;
  doc["raw"] = rawData;
  doc["success"] = true;
  doc["timestamp"] = millis() / 1000;

  char payload[1024];
  serializeJson(doc, payload);

  // 发布到MQTT
  String topic = MQTTClient::getTopic("learn/result");
  if (MQTTClient::publish(topic.c_str(), payload)) {
    DEBUG_PRINTLN("[学习] ✅ 学习结果已发布");
  } else {
    DEBUG_PRINTLN("[学习] ❌ 学习结果发布失败");
  }
}

void IRLearning::publishTimeout() {
  if (!MQTTClient::isConnected())
    return;

  // 构建超时消息
  StaticJsonDocument<256> doc;
  doc["key"] = learningKey;
  doc["success"] = false;
  doc["error"] = "timeout";

  char payload[256];
  serializeJson(doc, payload);

  // 发布到MQTT
  String topic = MQTTClient::getTopic("learn/result");
  MQTTClient::publish(topic.c_str(), payload);
}
