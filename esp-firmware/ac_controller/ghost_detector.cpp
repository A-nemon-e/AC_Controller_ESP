/*
 * Ghost检测器模块 - 实现
 */

#include "ghost_detector.h"
#include "config_manager.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>


// 静态成员初始化
MicConfig GhostDetector::micConfig = {true, 50, 500, "short"};
unsigned long GhostDetector::lastIRTime = 0;
unsigned long GhostDetector::lastMicTime = 0;
bool GhostDetector::lastMicState = LOW;

void GhostDetector::init() {
  DEBUG_PRINTLN("[Ghost] 初始化Ghost检测器");

  // 配置麦克风引脚
  pinMode(PIN_MIC, INPUT);

  DEBUG_PRINTLN("[Ghost] ✅ Ghost检测器就绪");
}

void GhostDetector::update() {
  if (!micConfig.enabled)
    return;

  // 读取麦克风状态
  bool micState = digitalRead(PIN_MIC);

  // 检测上升沿（麦克风触发）
  if (micState == HIGH && lastMicState == LOW) {
    onMicTriggered();
  }

  lastMicState = micState;
}

void GhostDetector::onIRReceived() {
  lastIRTime = millis();

  if (!micConfig.enabled)
    return;

  // 检查麦克风是否在时间窗口内触发
  DeviceConfig &cfg = ConfigManager::getConfig();
  unsigned long timeSinceMic = lastIRTime - lastMicTime;

  if (timeSinceMic > cfg.ghostWindow) {
    // 红外收到但麦克风未在窗口内触发 → Ghost!
    DEBUG_PRINTLN("[Ghost] 👻 检测到Ghost操作！");
    DEBUG_PRINTF("[Ghost] 距上次麦克风触发: %lu ms\n", timeSinceMic);
    publishGhostEvent();
  } else {
    DEBUG_PRINTLN("[Ghost] ✅ 正常操作（麦克风已触发）");
  }
}

void GhostDetector::onMicTriggered() {
  lastMicTime = millis();
  DEBUG_PRINTLN("[Ghost] 🎤 麦克风触发");
}

void GhostDetector::updateMicConfig(bool enabled, uint8_t sensitivity,
                                    uint16_t beepDuration,
                                    const char *beepType) {
  micConfig.enabled = enabled;
  micConfig.sensitivity = sensitivity;
  micConfig.beepDurationMs = beepDuration;
  micConfig.beepType = String(beepType);

  DEBUG_PRINTLN("[Ghost] 配置已更新");
  DEBUG_PRINTF("[Ghost] 启用: %s, 灵敏度: %d\n", enabled ? "是" : "否",
               sensitivity);
}

bool GhostDetector::isEnabled() { return micConfig.enabled; }

void GhostDetector::publishGhostEvent() {
  if (!MQTTClient::isConnected())
    return;

  // 构建Ghost事件消息
  StaticJsonDocument<256> doc;
  doc["type"] = "ghost";
  doc["timestamp"] = millis() / 1000;
  doc["timeSinceMic"] = lastIRTime - lastMicTime;

  char payload[256];
  serializeJson(doc, payload);

  // 发布到MQTT
  String topic = MQTTClient::getTopic("event");
  if (MQTTClient::publish(topic.c_str(), payload)) {
    DEBUG_PRINTLN("[Ghost] ✅ Ghost事件已发布");
  }
}
