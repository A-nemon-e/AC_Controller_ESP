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

#include "i2c_scanner.h" // I2C设备扫描工具
// #include "led_driver.h"  // LED矩阵驱动 (已弃用，使用DisplayEngine替代)
#include "sensors.h"
#include "state_manager.h"
#include "wifi_manager.h"
#include <ArduinoJson.h> // ✅ 新增：JSON库
#include <Wire.h>        // I2C库（用于AHT20和IS31FL3733）

// ✅ 新增：EasyButton 按键处理
#include <EasyButton.h>

// ✅ 新增：LED矩阵和字体渲染（DisplayEngine依赖）
#include <IS31FL3733.h>
#include <LEDMatrix.h>
#include <FontRenderer.h>

// ✅ 新增：DisplayEngine显示引擎
#include <DisplayEngine.h>
#include <DisplayConfig.h>

// ✅ 新增：非阻塞显示调度器（放在DisplayEngine之后）
#include <Core/DisplayTaskScheduler.h>

// ===== 全局变量定义 =====
// 定时器配置（可通过MQTT动态修改）
uint32_t sensorInterval = DEFAULT_SENSOR_INTERVAL;
uint32_t heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
uint32_t ghostWindow = DEFAULT_GHOST_WINDOW;

// ✅ 新增：按键和显示相关全局变量
bool screenOn = true;
uint8_t gccValue = 159;
int8_t brtDir = -1;

// ✅ 新增：NTP同步相关变量
bool ntpSynced = false;
uint32_t ntpLastSyncMs = 0;
bool wifiWasConnected = false;

// ===== 函数声明 =====
void onMQTTMessage(char *topic, uint8_t *payload, unsigned int length);
void onIRReceived(decode_results *results);
void handleConfigUpdate(const char *json);
void handleControlCommand(const char *json);
void handleLearnCommand(const char *json);
void handleAutoDetectCommand(const char *json);   // ✅ 新增
void handleConfigBindingUpdate(const char *json); // ✅ 新增：处理绑定配置
void printSystemInfo();
void publishDeviceAnnounce();                   // ✅ 设备上线消息
bool tryParseProtocol(decode_results *results); // ✅ 协议解析
void publishIREvent(decode_results *results);   // ✅ 红外事件上报

// ✅ 新增：EasyButton 配置和回调
#define BTN_DEBOUNCE_MS 35
#define BTN_LONG_MS 800
#define BTN_DOUBLE_MS 400
#define BTN_BRT_MS 30

void setupButton();
void processButton();
void doShortPress();
void doDoubleClick();
void doLongPressStep();

// ✅ 新增：DisplayEngine相关函数
void createDefaultCards();                      // 创建默认显示卡片

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

  // 3.5 初始化LED显示系统（DisplayEngine - 42×11宽屏）
  DEBUG_PRINTLN("[主程序] 初始化LED显示系统...");

  // 3.6 初始化DisplayEngine显示引擎（内部会处理I2C和SDB初始化）
  DisplayManager::getInstance().begin();
  DEBUG_PRINTLN("[主程序] ✅ DisplayEngine初始化完成");

  // 3.7 创建默认卡片（时钟卡片）
  createDefaultCards();
  DEBUG_PRINTLN("[主程序] ✅ 默认卡片创建完成");

  // 3.8 设置初始亮度
  LEDMatrix::setGlobalBrightness(gccValue);
  DEBUG_PRINTLN("[主程序] ✅ 初始亮度设置完成");

  // 3.9 启动非阻塞显示调度器（60Hz）
  DEBUG_PRINTLN("[主程序] 启动显示调度器...");
  DisplayTaskScheduler::getInstance().begin();
  DisplayTaskScheduler::getInstance().setRenderCallback([]() {
    // 在 Ticker 回调中执行渲染
    DisplayManager::getInstance().update();
    if (screenOn) {
      DisplayManager::getInstance().render();
    } else {
      LEDMatrix::clear();
      LEDMatrix::refresh();
    }
  });
  DEBUG_PRINTLN("[主程序] ✅ 显示调度器已启动 (60Hz)");

  // 3.10 显示WiFi连接提示（在阻塞连接前显示）
  DEBUG_PRINTLN("[主程序] 显示WiFi连接提示...");
  displayWiFiConnecting();

  // 4. 连接WiFi
  WiFiManager::connect();

  // 等待WiFi连接
  while (!WiFiManager::isConnected()) {
    delay(100);
    WiFiManager::maintain();
    LEDIndicator::update();
  }

  // ✅ 新增：WiFi连接成功后启动NTP同步
  DEBUG_PRINTLN("[NTP] 开始异步同步时钟...");
  configTime(8 * 3600, 0, "ntp.aliyun.com", "time.nist.gov");
  ntpSynced = false;

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

  // ✅ 新增：9. 初始化按键处理
  DEBUG_PRINTLN("[主程序] 初始化按键处理...");
  setupButton();
  DEBUG_PRINTLN("[主程序] ✅ 按键处理初始化完成");

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
  // ✅ 高优先级：检查并执行渲染（由 Ticker ISR 触发）
  // 这样渲染固定在 60fps，同时主循环保持高频率运行
  DisplayTaskScheduler::getInstance().update();

  // 维护WiFi连接
  WiFiManager::maintain();

  // ✅ 新增：检测WiFi连接状态变化
  bool currentWifiConnected = WiFiManager::isConnected();
  if (currentWifiConnected && !wifiWasConnected) {
    // WiFi刚连接，启动NTP同步
    DEBUG_PRINTLN("[NTP] WiFi已连接，启动NTP同步...");
    configTime(8 * 3600, 0, "ntp.aliyun.com", "time.nist.gov");
    ntpSynced = false;
  }
  wifiWasConnected = currentWifiConnected;

  // ✅ 新增：检查NTP同步状态
  if (wifiWasConnected && !ntpSynced && time(nullptr) > 1000000000UL) {
    Serial.println("[NTP] 同步成功");
    ntpSynced = true;
    ntpLastSyncMs = millis();
  }

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

  // ✅ 新增：处理按键
  processButton();

  // 更新传感器（定时上报）
  Sensors::update();

  // 处理红外接收
  IRController::handleReceive();

  // 更新学习模式
  IRLearning::update();

  // 更新Ghost检测
  GhostDetector::update();

  // 让出CPU，避免看门狗复位
  // 使用 delay(0) 比 delay(1) 更频繁地让出时间，减少卡顿
  delay(0);
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

  // ===== ✅ 优先级1: 品牌协议模式 (配置) =====
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

// ===== 创建默认显示卡片 =====
void createDefaultCards() {
  DEBUG_PRINTLN("[DisplayEngine] 创建默认卡片...");

  // 卡片1: 矩阵雨背景+时钟前景
  Card* matrixCard = new Card("矩阵雨");
  MatrixRainBackground* matrixBg = new MatrixRainBackground();
  matrixCard->setBackground(matrixBg);
  ClockForeground* clockFg1 = new ClockForeground();
  clockFg1->setFormat(ClockForeground::Format::HH_MM);
  clockFg1->setPosition(Position::TOP_CENTER);
  clockFg1->setFont(FontType::FONT_3x5);
  clockFg1->setBrightness(200);
  matrixCard->addForeground(clockFg1);
  DisplayManager::getInstance().addCard(matrixCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片1: 矩阵雨已创建");

  // 卡片2: 水波纹背景（无时钟前景）
  Card* waterCard = new Card("水波纹");
  WaterRippleBackground* waterBg = new WaterRippleBackground();
  waterCard->setBackground(waterBg);
  DisplayManager::getInstance().addCard(waterCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片2: 水波纹已创建");

  // 卡片3: 生命游戏背景（无时钟前景）
  Card* lifeCard = new Card("生命游戏");
  GameOfLifeBackground* lifeBg = new GameOfLifeBackground();
  lifeCard->setBackground(lifeBg);
  DisplayManager::getInstance().addCard(lifeCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片3: 生命游戏已创建");

  // 卡片4: 火焰背景+时钟前景
  Card* fireCard = new Card("火焰");
  FireBackground* fireBg = new FireBackground();
  fireCard->setBackground(fireBg);
  ClockForeground* clockFg4 = new ClockForeground();
  clockFg4->setFormat(ClockForeground::Format::HH_MM);
  clockFg4->setPosition(Position::TOP_CENTER);
  clockFg4->setFont(FontType::FONT_3x5);
  clockFg4->setBrightness(220);
  fireCard->addForeground(clockFg4);
  DisplayManager::getInstance().addCard(fireCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片4: 火焰已创建");

  // 卡片5: 沙漏背景（无时钟前景）
  Card* sandCard = new Card("沙漏");
  SandBackground* sandBg = new SandBackground();
  sandCard->setBackground(sandBg);
  DisplayManager::getInstance().addCard(sandCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片5: 沙漏已创建");

  // 卡片6: Pong背景（不显示时钟）
  Card* pongCard = new Card("Pong");
  PongBackground* pongBg = new PongBackground();
  pongCard->setBackground(pongBg);
  DisplayManager::getInstance().addCard(pongCard);
  DEBUG_PRINTLN("[DisplayEngine] 卡片6: Pong已创建");

  DEBUG_PRINTF("[DisplayEngine] 共创建 %d 个默认卡片\n",
               DisplayManager::getInstance().getCardCount());
}

// ===== 显示WiFi连接提示 =====
void displayWiFiConnecting() {
  // 使用FontRenderer在左上角显示"WiFi"
  LEDMatrix::clear();
  
  // 在左上角显示 "WiFi" 字样
  FontRenderer::drawString("WiFi", 0, 0, FontType::FONT_3x5, 200);
  
  LEDMatrix::refresh();
  DEBUG_PRINTLN("[显示] WiFi Connecting 提示已显示");
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

// ✅ 新增：EasyButton 按键处理实现

// 注意：全局变量 screenOn, gccValue, brtDir 已在文件开头定义

// EasyButton 实例
EasyButton btn(PIN_BUTTON, BTN_DEBOUNCE_MS, false, false);

// 按键状态变量
static bool _brtBounced = false;
static uint32_t _brtMs = 0;
static bool _pendingShort = false;
static uint32_t _pendingMs = 0;

void doShortPress() {
  screenOn = !screenOn;
  applyBrightness();
  DEBUG_PRINTF("[按键] 短按 → 屏幕%s\n", screenOn ? "开" : "关");
}

void doDoubleClick() {
  if (!screenOn) return;
  DisplayManager::getInstance().nextCard();
  Card* currentCard = DisplayManager::getInstance().getCurrentCard();
  if (currentCard) {
    DEBUG_PRINTF("[按键] 双击 → 切换到卡片: %s\n", currentCard->getName());
  }
}

void doLongPressStep() {
  if (!screenOn || _brtBounced) return;
  int step = max(1, (int)(8.0f * sqrtf((float)gccValue / 255.0f) + 0.5f));
  int v = (int)gccValue + brtDir * step;
  if (v >= 255) {
    v = 255;
    _brtBounced = true;
  } else if (v <= 1) {
    v = 1;
    _brtBounced = true;
  }
  gccValue = (uint8_t)v;
  applyBrightness();
  DEBUG_PRINTF("[按键] 长按调节 → GCC=%d(step=%d)\n", gccValue, step);
}

void applyBrightness() {
  LEDMatrix::setGlobalBrightness(screenOn ? gccValue : 0);
}

void setupButton() {
  btn.onPressed([]() {
    _pendingShort = true;
    _pendingMs = millis();
  });

  btn.onSequence(2, BTN_DOUBLE_MS, []() {
    if (!screenOn) return;
    _pendingShort = false;
    doDoubleClick();
  });

  btn.onPressedFor(BTN_LONG_MS, []() {
    _pendingShort = false;
    if (!screenOn) return;
    _brtBounced = false;
    brtDir = -brtDir;
    _brtMs = millis() - BTN_BRT_MS;
    DEBUG_PRINTF("[按键] 长按开始 → dir=%d\n", brtDir);
  });

  btn.begin();
  DEBUG_PRINTLN("[按键] EasyButton 初始化完成");
}

void processButton() {
  btn.read();

  // 处理短按（等待双击超时）
  if (_pendingShort && millis() - _pendingMs >= BTN_DOUBLE_MS) {
    _pendingShort = false;
    doShortPress();
  }

  // 处理长按亮度调节
  if (screenOn && btn.pressedFor(BTN_LONG_MS)) {
    if (!_brtBounced && millis() - _brtMs >= BTN_BRT_MS) {
      _brtMs = millis();
      doLongPressStep();
    }
  }

  if (btn.wasReleased()) {
    _brtBounced = false;
  }
}

