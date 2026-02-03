/*
 * ESP12F空调控制器 - 主程序
 *
 * 功能概述：
 * - 动态配置管理（支持OTA配置更新）
 * - WiFi连接管理
 * - MQTT通信
 * - 红外收发控制
 * - 传感器数据采集
 * - Ghost检测
 * - 学习功能
 * - 状态管理
 *
 * 作者：AI Assistant
 * 版本：v1.3.0 - 阶段三（学习 + Ghost + 状态管理）
 */

#include "auto_detect.h" // ✅ 新增：自动协议检测
#include "config.h"
#include "config_manager.h"
#include "ghost_detector.h"
#include "ir_controller.h"
#include "ir_learning.h"
#include "led_indicator.h"
#include "mqtt_client.h"
#include "sensors.h"
#include "state_manager.h"
#include "wifi_manager.h"
#include <ArduinoJson.h> // ✅ 新增：JSON库


// ===== 全局变量定义 =====
// 定时器配置（可通过MQTT动态修改）
uint32_t sensorInterval = DEFAULT_SENSOR_INTERVAL;
uint32_t heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
uint32_t ghostWindow = DEFAULT_GHOST_WINDOW;

// ===== 函数声明 =====
void onMQTTMessage(char *topic, uint8_t *payload, unsigned int length);
void onIRReceived(decode_results *results);
void handleConfigUpdate(const char *json);
void handleControlCommand(const char *json);
void handleLearnCommand(const char *json);
void handleAutoDetectCommand(const char *json); // ✅ 新增
void printSystemInfo();

// ===== 初始化 =====
void setup() {
  // 1. 初始化串口
  Serial.begin(SERIAL_BAUD);
  delay(100);

  DEBUG_PRINTLN();
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("  ESP12F 空调控制器");
  DEBUG_PRINTLN("  版本: v1.3.0 (完整功能)");
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN();

  // 2. 初始化配置管理器
  ConfigManager::init();
  ConfigManager::printConfig();

  // 3. 初始化LED指示
  LEDIndicator::init();

  // 4. 连接WiFi
  WiFiManager::connect();

  // 等待WiFi连接
  while (!WiFiManager::isConnected()) {
    delay(100);
    WiFiManager::maintain();
    LEDIndicator::update();
  }

  // 5. 连接MQTT
  MQTTClient::setCallback(onMQTTMessage);
  MQTTClient::connect();

  // 等待MQTT连接
  while (!MQTTClient::isConnected()) {
    delay(100);
    MQTTClient::loop();
    LEDIndicator::update();
  }

  // 6. 初始化传感器
  if (Sensors::init()) {
    DEBUG_PRINTLN("[主程序] ✅ 传感器初始化成功");
  } else {
    DEBUG_PRINTLN("[主程序] ⚠️  传感器初始化失败，部分功能不可用");
  }

  // 7. 初始化红外控制器
  IRController::init();
  IRController::setReceiveCallback(onIRReceived);

  // 8. 初始化Ghost检测器
  GhostDetector::init();

  // 9. 初始化状态管理器
  StateManager::init();

  // 10. 订阅配置更新topic（基于MAC地址）
  String configTopic = "ac/config/" + WiFi.macAddress();
  configTopic.replace(":", "");
  MQTTClient::subscribe(configTopic.c_str());
  DEBUG_PRINTF("[主程序] 已订阅配置topic: %s\n", configTopic.c_str());

  // 11. 打印系统信息
  printSystemInfo();

  DEBUG_PRINTLN();
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("  系统初始化完成！");
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN();
}

// ===== 主循环 =====
void loop() {
  // 维护WiFi连接
  WiFiManager::maintain();

  // 维护MQTT连接
  MQTTClient::loop();

  // 更新LED状态
  LEDIndicator::update();

  // 更新传感器（定时上报）
  Sensors::update();

  // 处理红外接收
  IRController::handleReceive();

  // 更新学习模式
  IRLearning::update();

  // 更新Ghost检测
  GhostDetector::update();

  // 短暂延时
  delay(10);
}

// ===== 红外接收回调 =====
void onIRReceived(decode_results *results) {
  DEBUG_PRINTLN("[主程序] 红外接收回调触发");

  // ===== 优先级1: 自动检测模式 =====
  if (AutoDetect::isDetecting()) {
    DEBUG_PRINTLN("[主程序] 进入自动检测分析");

    DetectionResult result = AutoDetect::analyze(results);

    // 构建MQTT消息
    StaticJsonDocument<768> doc;
    doc["success"] = result.success;
    doc["protocol"] = result.protocol;
    doc["model"] = result.model;

    if (result.success && result.isAC) {
      doc["isAC"] = true;
      doc["power"] = result.power;
      doc["mode"] = result.mode;
      doc["temp"] = result.temp;
      doc["fan"] = result.fan;
      doc["swingV"] = result.swingV;
      doc["swingH"] = result.swingH;
      doc["description"] = result.description;

      DEBUG_PRINTLN("[主程序] ✅ AC协议识别成功，上报结果");
    } else if (!result.success) {
      doc["rawData"] = result.rawData;
      DEBUG_PRINTLN("[主程序] ❌ 协议未识别，返回raw数据");
    }

    char payload[768];
    serializeJson(doc, payload);

    String topic = MQTTClient::getTopic("auto_detect/result");
    MQTTClient::publish(topic.c_str(), payload);

    // 自动停止检测
    AutoDetect::stop();

    return; // 不继续其他处理
  }

  // ===== 优先级2: 学习模式 =====
  if (IRLearning::isLearning()) {
    IRLearning::onIRReceived(results);
    return;
  }

  // ===== 优先级3: 正常模式（Ghost检测）=====
  // 触发Ghost检测
  GhostDetector::onIRReceived();

  // TODO: 解析红外数据更新空调状态
  // 目前仅记录source为ir_recv
  StateManager::setState(true,         // 假设收到信号表示开机
                         "cool",       // 默认模式
                         26,           // 默认温度
                         0,            // 默认风速
                         false, false, // 扫风
                         "ir_recv"     // 来源
  );
}

// ===== MQTT消息回调函数 =====
void onMQTTMessage(char *topic, uint8_t *payload, unsigned int length) {
  DEBUG_PRINTLN("[主程序] 处理MQTT消息");

  // 转换payload为字符串
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  String topicStr = String(topic);
  String msgStr = String(message);

  DEBUG_PRINTF("[主程序] Topic: %s\n", topic);
  DEBUG_PRINTF("[主程序] Message: %s\n", message);

  // 检查是否是配置更新消息
  if (topicStr.indexOf("/config/") >= 0) {
    DEBUG_PRINTLN("[主程序] → 收到配置更新");
    handleConfigUpdate(message);
    return;
  }

  // 处理其他命令
  if (topicStr.endsWith("/cmd")) {
    handleControlCommand(message);

  } else if (topicStr.endsWith("/learn/start")) {
    handleLearnCommand(message);

  } else if (topicStr.endsWith("/config")) {
    handleConfigUpdate(message);

  } else if (topicStr.endsWith("/auto_detect")) { // ✅ 新增
    handleAutoDetectCommand(message);
  }
}

// ===== 处理控制命令 =====
void handleControlCommand(const char *json) {
  DEBUG_PRINTLN("[主程序] → 收到控制命令");

  // 解析JSON命令
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINTLN("[主程序] ❌ JSON解析失败");
    return;
  }

  // 解析空调状态参数
  bool power = doc["power"] | false;
  const char *mode = doc["mode"] | "cool";
  uint8_t temp = doc["temp"] | 26;
  uint8_t fan = doc["fan"] | 0;
  bool swingV = doc["swingVertical"] | false;
  bool swingH = doc["swingHorizontal"] | false;

  // ===== ✅ 优先级1: 品牌协议模式 =====
  DeviceConfig &cfg = ConfigManager::getConfig();
  if (cfg.brand[0] != '\0') { // 如果配置了品牌
    DEBUG_PRINTF("[主程序] 使用品牌协议: %s\n", cfg.brand);

    if (IRController::sendBrand(cfg.brand, cfg.model, power, mode, temp, fan,
                                swingV, swingH)) {
      // 发送成功，更新状态
      StateManager::setState(power, mode, temp, fan, swingV, swingH, "api");
      DEBUG_PRINTLN("[主程序] ✅ 品牌协议命令已发送");
      return;
    } else {
      DEBUG_PRINTLN("[主程序] ⚠️ 品牌协议发送失败，尝试raw模式");
      // 继续尝试raw模式
    }
  }

  // ===== ⚠️ 优先级2: Raw模式（降级） =====
  if (doc.containsKey("raw")) {
    const char *rawData = doc["raw"];
    DEBUG_PRINTLN("[主程序] 使用raw红外数据");
    IRController::sendRaw(rawData);

    // 更新状态（从JSON）
    StateManager::updateFromJSON(json);
    DEBUG_PRINTLN("[主程序] ✅ Raw命令已发送");
    return;
  }

  // ===== ❌ 降级：只记录状态 =====
  DEBUG_PRINTLN("[主程序] ⚠️ 无品牌配置且无raw数据，只记录状态");
  StateManager::setState(power, mode, temp, fan, swingV, swingH, "api");
}

// ===== 处理学习命令 =====
void handleLearnCommand(const char *json) {
  DEBUG_PRINTLN("[主程序] → 收到学习指令");

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (!error && doc.containsKey("key")) {
    const char *key = doc["key"];
    IRLearning::start(key);
  } else {
    DEBUG_PRINTLN("[主程序] ❌ 学习指令格式错误");
  }
}

// ===== 处理配置更新 =====
void handleConfigUpdate(const char *json) {
  DEBUG_PRINTLN("[配置更新] 处理配置JSON");

  if (ConfigManager::updateFromJSON(json)) {
    DEBUG_PRINTLN("[配置更新] ✅ 配置更新成功");

    // 打印新配置
    ConfigManager::printConfig();

    // 发布配置确认消息
    String ackTopic = "ac/config_ack/" + WiFi.macAddress();
    ackTopic.replace(":", "");
    MQTTClient::publish(ackTopic.c_str(),
                        "{\"status\":\"ok\",\"updated\":true}");
  } else {
    DEBUG_PRINTLN("[配置更新] ❌ 配置更新失败");
  }
}

// ===== 打印系统信息 =====
void printSystemInfo() {
  DeviceConfig &cfg = ConfigManager::getConfig();

  DEBUG_PRINTLN("\n[系统信息]");
  DEBUG_PRINTF("  芯片ID: 0x%08X\n", ESP.getChipId());
  DEBUG_PRINT("  MAC地址: ");
  DEBUG_PRINTLN(WiFiManager::getMACAddress());
  DEBUG_PRINT("  IP地址: ");
  DEBUG_PRINTLN(WiFiManager::getIPAddress());
  DEBUG_PRINT("  空闲内存: ");
  DEBUG_PRINT(ESP.getFreeHeap());
  DEBUG_PRINTLN(" bytes");
  DEBUG_PRINT("  设备UUID: ");
  DEBUG_PRINTLN(cfg.deviceUUID);
  DEBUG_PRINTF("  用户ID: %u\n", cfg.userId);
  DEBUG_PRINTF("  MQTT服务器: %s:%d\n", cfg.mqttServer, cfg.mqttPort);
}

// ===== 处理自动检测命令 =====
void handleAutoDetectCommand(const char *json) {
  DEBUG_PRINTLN("[主程序] → 收到自动检测指令");

  // 解析JSON (可选，目前只需要action字段)
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINTLN("[自动检测] ❌ JSON解析失败");
    return;
  }

  const char *action = doc["action"] | "start";

  if (strcmp(action, "start") == 0) {
    // 启动自动检测
    AutoDetect::start();

    // 上报状态
    StaticJsonDocument<128> statusDoc;
    statusDoc["status"] = "detecting";
    statusDoc["timeout"] = 30;
    statusDoc["message"] = "请在30秒内按下遥控器任意键";

    char payload[128];
    serializeJson(statusDoc, payload);

    String topic = MQTTClient::getTopic("auto_detect/status");
    MQTTClient::publish(topic.c_str(), payload);

    DEBUG_PRINTLN("[自动检测] ✅ 已启动，等待红外信号");
  } else if (strcmp(action, "stop") == 0) {
    // 停止自动检测
    AutoDetect::stop();

    StaticJsonDocument<64> statusDoc;
    statusDoc["status"] = "idle";

    char payload[64];
    serializeJson(statusDoc, payload);

    String topic = MQTTClient::getTopic("auto_detect/status");
    MQTTClient::publish(topic.c_str(), payload);

    DEBUG_PRINTLN("[自动检测] ⏹ 已停止");
  }
}
