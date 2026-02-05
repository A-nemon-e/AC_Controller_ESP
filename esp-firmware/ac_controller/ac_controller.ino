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
#include "scene_manager.h" // ✅ 新增：学习场景管理
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
void handleAutoDetectCommand(const char *json);   // ✅ 新增
void handleConfigBindingUpdate(const char *json); // ✅ 新增：处理绑定配置
void handleSceneSaveCommand(const char *json);    // ✅ 新增：处理场景保存
void printSystemInfo();
void publishDeviceAnnounce();                       // ✅ 设备上线消息
bool tryParseProtocol(decode_results *results);     // ✅ 协议解析
bool tryMatchLearnedScene(decode_results *results); // ✅ 场景匹配
void publishIREvent(decode_results *results);       // ✅ 红外事件上报

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

  // 9. 初始化场景管理器
  SceneManager::init();

  // 10. 初始化状态管理器
  StateManager::init();

  // 10. 订阅配置更新topic（基于MAC地址）
  String configTopic = "ac/config/" + WiFi.macAddress();
  configTopic.replace(":", "");
  MQTTClient::subscribe(configTopic.c_str());
  DEBUG_PRINTF("[主程序] 已订阅配置topic: %s\n", configTopic.c_str());

  // 11. 打印系统信息
  printSystemInfo();

  // 12. 发送设备上线消息 -> 移动到 loop 中检测到连接后发送
  // publishDeviceAnnounce();

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

  // ✅ 新增：检测MQTT连接状态变化，发送上线/发现消息
  static bool lastMqttConnected = false;
  bool currentMqttConnected = MQTTClient::isConnected();

  if (currentMqttConnected && !lastMqttConnected) {
    DEBUG_PRINTLN("[主程序] MQTT已连接，发送上线消息...");
    publishDeviceAnnounce();
  }
  lastMqttConnected = currentMqttConnected;

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
    // ✅ 改为堆分配，防止Stack Smashing
    DynamicJsonDocument doc(2048);
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

    // ✅ 改为String (堆分配)，防止Stack Overflow
    String payload;
    serializeJson(doc, payload);

    String topic = MQTTClient::getTopic("auto_detect/result");
    MQTTClient::publish(topic.c_str(), payload.c_str());

    // 自动停止检测
    AutoDetect::stop();

    return; // 不继续其他处理
  }

  // ===== 优先级2: 学习模式 =====
  if (IRLearning::isLearning()) {
    IRLearning::onIRReceived(results);
    return;
  }

  // ===== 优先级3: 正常模式 - 完整解析 =====

  // 触发Ghost检测
  GhostDetector::onIRReceived();

  // 尝试协议解析
  if (tryParseProtocol(results)) {
    DEBUG_PRINTLN("[主程序] ✅ 协议解析成功，状态已更新");
    return;
  }

  // 尝试匹配学习场景
  if (tryMatchLearnedScene(results)) {
    DEBUG_PRINTLN("[主程序] ✅ 匹配到学习场景，状态已更新");
    return;
  }

  // 都不匹配，只发布事件
  DEBUG_PRINTLN("[主程序] ⚠️ 无法解析，只发布事件");
  publishIREvent(results);
}

// ===== 尝试协议解析 =====
bool tryParseProtocol(decode_results *results) {
  DeviceConfig &cfg = ConfigManager::getConfig();

  // 检查是否配置了品牌
  if (cfg.brand[0] == '\0') {
    DEBUG_PRINTLN("[协议解析] 未配置品牌，跳过");
    return false;
  }

  // 使用 IRremoteESP8266 库解析
  // 根据协议类型提取参数
  IRac ac(PIN_IR_SEND);

  // 检查是否为空调协议
  if (!ac.isProtocolSupported(results->decode_type)) {
    DEBUG_PRINTF("[协议解析] 协议 %d 不是空调协议\n", results->decode_type);
    return false;
  }

  // 尝试解析状态（IRremoteESP8266的高级功能）
  stdAc::state_t state;
  if (!IRAcUtils::decodeToState(results, &state)) {
    DEBUG_PRINTLN("[协议解析] ❌ 状态解析失败");
    return false;
  }

  // 提取参数并更新状态
  bool power = state.power;
  const char *mode = "cool";

  // 转换模式
  switch (state.mode) {
  case stdAc::opmode_t::kCool:
    mode = "cool";
    break;
  case stdAc::opmode_t::kHeat:
    mode = "heat";
    break;
  case stdAc::opmode_t::kFan:
    mode = "fan";
    break;
  case stdAc::opmode_t::kDry:
    mode = "dry";
    break;
  case stdAc::opmode_t::kAuto:
    mode = "auto";
    break;
  default:
    mode = "cool";
  }

  uint8_t temp = state.degrees;
  uint8_t fan = (uint8_t)state.fanspeed;
  bool swingV = state.swingv != stdAc::swingv_t::kOff;
  bool swingH = state.swingh != stdAc::swingh_t::kOff;

  // 更新状态
  StateManager::setState(power, mode, temp, fan, swingV, swingH, "ir_protocol");

  DEBUG_PRINTF("[协议解析] ✅ 解析成功: %s %s %d°C\n", power ? "开" : "关",
               mode, temp);

  return true;
}

// ===== 尝试匹配学习场景 =====
bool tryMatchLearnedScene(decode_results *results) {
  String rawData = IRController::getLastRawData(); // 使用IRController的方法
  LearnedScene scene;

  if (SceneManager::matchScene(rawData.c_str(), scene)) {
    // 匹配成功，使用场景参数更新状态
    StateManager::setState(scene.power, scene.mode, scene.temp, 0, false, false,
                           "ir_learned");

    DEBUG_PRINTF("[场景匹配] ✅ 匹配到: %s\n", scene.key);
    return true;
  }

  DEBUG_PRINTLN("[场景匹配] ❌ 未匹配到任何场景");
  return false;
}

// ===== 发布红外事件 =====
void publishIREvent(decode_results *results) {
  DEBUG_PRINTLN("[主程序] 发布红外事件");
  StaticJsonDocument<512> doc;
  doc["type"] = "ir_event";
  doc["protocol"] =
      typeToString(results->decode_type); // 使用IRremoteESP8266的函数
  doc["value"] = uint64ToString(results->value, 16);
  doc["bits"] = results->bits;
  doc["rawData"] = IRController::getLastRawData(); // 使用IRController的方法

  char payload[512];
  serializeJson(doc, payload);

  String topic = MQTTClient::getTopic("ir_event");
  MQTTClient::publish(topic.c_str(), payload);
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

  // ✅ 优先处理：设备绑定配置 (包含 /config/update)
  if (topicStr.indexOf("/config/update") >= 0) {
    DEBUG_PRINTLN("[主程序] → 收到设备绑定配置");
    handleConfigBindingUpdate(message);
    return;
  }

  // 检查是否是配置更新消息 (包含 /config/)
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

  } else if (topicStr.endsWith("/auto_detect")) {
    handleAutoDetectCommand(message);

  } else if (topicStr.endsWith("/brands/get")) { // ✅ 新增：获取品牌列表
    DEBUG_PRINTLN("[主程序] → 请求品牌列表");
    String json = IRController::getSupportedBrandsJSON();
    String topic = MQTTClient::getTopic("brands/list");
    MQTTClient::publish(topic.c_str(), json.c_str());

  } else if (topicStr.endsWith("/scene/save")) { // ✅ 新增：批量保存场景
    handleSceneSaveCommand(message);
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

  // ✅ 修复: 优先使用 setTemp (新标准)，兼容 temp (旧标准)
  uint8_t temp = 26;
  if (doc.containsKey("setTemp")) {
    temp = doc["setTemp"];
  } else {
    temp = doc["temp"] | 26;
  }

  uint8_t fan = doc["fan"] | 0;
  bool swingV = doc["swingVertical"] | false;
  bool swingH = doc["swingHorizontal"] | false;

  // ===== ✅ 优先级0: 临时指令 (Ephemeral Command) =====
  if (doc.containsKey("brand")) {
    const char *ephBrand = doc["brand"];
    int ephModel = doc["model"] | 1; // 默认 Model 1
    DEBUG_PRINTF("[主程序] 收到临时测试指令: %s (Model=%d)\n", ephBrand,
                 ephModel);

    if (IRController::sendBrand(ephBrand, ephModel, power, mode, temp, fan,
                                swingV, swingH)) {
      DEBUG_PRINTLN("[主程序] ✅ 临时指令发送成功");
      return;
    } else {
      DEBUG_PRINTLN("[主程序] ❌ 临时指令发送失败 (不支持的协议?)");
    }
  }

  // ===== ✅ 优先级1: 品牌协议模式 (EEPROM配置) =====
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

// ===== 处理场景批量保存 (Front-end Wizard) =====
void handleSceneSaveCommand(const char *json) {
  DEBUG_PRINTLN("[主程序] → 收到场景保存指令");

  // 增大 buffer 以容纳 7 个场景的大 JSON
  // ✅ 改为堆分配 (4KB在栈上必挂)
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINTLN("[场景保存] ❌ JSON解析失败");
    return;
  }

  JsonArray scenes = doc["scenes"];
  if (scenes.isNull()) {
    DEBUG_PRINTLN("[场景保存] ❌ 缺少 scenes 数组");
    return;
  }

  DEBUG_PRINTLN("[场景保存] 清空旧场景...");
  SceneManager::clearScenes();

  int successCount = 0;
  for (JsonObject scene : scenes) {
    const char *key = scene["key"];
    const char *rawData = scene["raw"];
    bool power = scene["power"] | false;
    const char *mode = scene["mode"] | "cool";

    // ✅ 优先 setTemp
    uint8_t temp = 26;
    if (scene.containsKey("setTemp")) {
      temp = scene["setTemp"];
    } else {
      temp = scene["temp"] | 26;
    }

    if (key && rawData) {
      if (SceneManager::addScene(key, rawData, power, mode, temp)) {
        successCount++;
        DEBUG_PRINTF("[场景保存] 已保存: %s\n", key);
      }
    }
  }

  DEBUG_PRINTF("[场景保存] ✅ 成功保存 %d 个场景\n", successCount);

  // 发送确认 (可选)
  // String topic = MQTTClient::getTopic("scene/save_ack");
  // MQTTClient::publish(topic.c_str(), "{\"status\":\"ok\"}");
}

// ===== 发送设备上线消息（用于设备发现）=====
void publishDeviceAnnounce() {
  DeviceConfig &cfg = ConfigManager::getConfig();

  // 只有未绑定设备才发送发现消息（userId == 0）
  if (cfg.userId == 0) {
    DEBUG_PRINTLN("[设备发现] 发送上线消息...");

    // 构建JSON消息
    StaticJsonDocument<256> doc;
    doc["uuid"] = cfg.deviceUUID;
    doc["mac"] = WiFi.macAddress();
    doc["ip"] = WiFi.localIP().toString();
    doc["userId"] = cfg.userId;
    doc["brand"] = cfg.brand;
    doc["model"] = cfg.model;
    doc["timestamp"] = millis();

    char payload[256];
    serializeJson(doc, payload);

    // 发布到 ac/discovery/<UUID>
    String topic = "ac/discovery/" + String(cfg.deviceUUID);
    MQTTClient::publish(topic.c_str(), payload,
                        false); // ✅ 取消 Retained (User Request)

    DEBUG_PRINTLN("[设备发现] ✅ 上线消息已发送");
    DEBUG_PRINTF("[设备发现] Topic: %s\n", topic.c_str());
    DEBUG_PRINTF("[设备发现] Payload: %s\n", payload);
  } else {
    DEBUG_PRINTF("[设备发现] 设备已绑定（用户ID: %u），跳过发现消息\n",
                 cfg.userId);
  }
}

// ✅ 新增：处理设备绑定配置
void handleConfigBindingUpdate(const char *json) {
  DEBUG_PRINTLN("[绑定配置] 处理设备绑定配置");

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINTLN("[绑定配置] ❌ JSON解析失败");
    return;
  }

  // 提取userId和deviceId
  if (!doc.containsKey("userId") || !doc.containsKey("deviceId")) {
    DEBUG_PRINTLN("[绑定配置] ❌ 缺少必需字段");
    return;
  }

  uint32_t userId = doc["userId"];
  uint32_t deviceId = doc["deviceId"];

  DEBUG_PRINTF("[绑定配置] 收到绑定配置 - userID: %u, deviceID: %u\n", userId,
               deviceId);

  // 保存到EEPROM
  ConfigManager::saveUserId(userId);
  ConfigManager::saveDeviceId(deviceId);

  // ✅ 关键修复：必须调用 save() 更新结构体和校验和！
  // 否则下次启动 load() 会读取旧的结构体（UserId=0）且校验通过，覆盖正确的配置
  ConfigManager::save();

  DEBUG_PRINTLN("[绑定配置] ✅ 配置已保存到EEPROM");

  // 重新订阅MQTT topic（使用新的userId）
  MQTTClient::resubscribe();

  // 延迟一下，确保订阅生效
  delay(100);

  // 发布确认消息
  publishDeviceAnnounce();

  DEBUG_PRINTLN("[绑定配置] ✅ 设备绑定完成");
}
