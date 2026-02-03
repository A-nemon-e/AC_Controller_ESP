/*
 * 配置管理模块 - 实现
 */

#include "config_manager.h"
#include <ArduinoJson.h>

// 静态成员初始化
DeviceConfig ConfigManager::config;
bool ConfigManager::loaded = false;

void ConfigManager::init() {
  DEBUG_PRINTLN("[配置] 初始化配置管理器");

  // 初始化EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // 尝试加载配置
  if (!load()) {
    DEBUG_PRINTLN("[配置] 未找到有效配置，使用默认值");
    resetToDefault();
    save();
  }
}

bool ConfigManager::load() {
  DEBUG_PRINTLN("[配置] 从EEPROM加载配置...");

  // 读取配置结构体
  EEPROM.get(EEPROM_CONFIG_ADDR, config);

  // 验证校验和
  if (!verifyChecksum()) {
    DEBUG_PRINTLN("[配置] ❌ 校验和错误，配置无效");
    return false;
  }

  DEBUG_PRINTLN("[配置] ✅ 配置加载成功");
  loaded = true;
  return true;
}

bool ConfigManager::save() {
  DEBUG_PRINTLN("[配置] 保存配置到EEPROM...");

  // 计算校验和
  config.checksum = calculateChecksum();

  // 写入EEPROM
  EEPROM.put(EEPROM_CONFIG_ADDR, config);
  EEPROM.commit();

  DEBUG_PRINTLN("[配置] ✅ 配置已保存");
  return true;
}

void ConfigManager::resetToDefault() {
  DEBUG_PRINTLN("[配置] 重置为默认配置");

  // MQTT服务器配置（使用config.h中的默认值）
  strncpy(config.mqttServer, MQTT_SERVER, sizeof(config.mqttServer) - 1);
  config.mqttPort = MQTT_PORT;
  strncpy(config.mqttUser, MQTT_USER, sizeof(config.mqttUser) - 1);
  strncpy(config.mqttPassword, MQTT_PASSWORD, sizeof(config.mqttPassword) - 1);

  // 设备UUID：使用MAC地址生成
  String uuid = generateUUID();
  strncpy(config.deviceUUID, uuid.c_str(), sizeof(config.deviceUUID) - 1);

  // 用户ID：默认为0（未绑定）
  config.userId = 0;

  // 定时器配置
  config.sensorInterval = DEFAULT_SENSOR_INTERVAL;
  config.ghostWindow = DEFAULT_GHOST_WINDOW;

  // ✅ 品牌配置：默认为空（未配置）
  config.brand[0] = '\0';
  config.model = 0;

  // 计算校验和
  config.checksum = calculateChecksum();
}

bool ConfigManager::updateFromJSON(const char *json) {
  DEBUG_PRINTLN("[配置] 从JSON更新配置");
  DEBUG_PRINTF("[配置] JSON: %s\n", json);

  // 解析JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINT("[配置] ❌ JSON解析失败: ");
    DEBUG_PRINTLN(error.c_str());
    return false;
  }

  // 更新配置项（如果存在）
  bool changed = false;

  if (doc.containsKey("mqttServer")) {
    strncpy(config.mqttServer, doc["mqttServer"],
            sizeof(config.mqttServer) - 1);
    changed = true;
  }

  if (doc.containsKey("mqttPort")) {
    config.mqttPort = doc["mqttPort"];
    changed = true;
  }

  if (doc.containsKey("mqttUser")) {
    strncpy(config.mqttUser, doc["mqttUser"], sizeof(config.mqttUser) - 1);
    changed = true;
  }

  if (doc.containsKey("mqttPassword")) {
    strncpy(config.mqttPassword, doc["mqttPassword"],
            sizeof(config.mqttPassword) - 1);
    changed = true;
  }

  if (doc.containsKey("deviceUUID")) {
    strncpy(config.deviceUUID, doc["deviceUUID"],
            sizeof(config.deviceUUID) - 1);
    changed = true;
  }

  if (doc.containsKey("userId")) {
    config.userId = doc["userId"];
    changed = true;
  }

  if (doc.containsKey("sensorInterval")) {
    config.sensorInterval = doc["sensorInterval"];
    changed = true;
  }

  if (doc.containsKey("ghostWindow")) {
    config.ghostWindow = doc["ghostWindow"];
    changed = true;
  }

  // ✅ 品牌协议配置
  if (doc.containsKey("brand")) {
    const char *brandStr = doc["brand"];
    strncpy(config.brand, brandStr, sizeof(config.brand) - 1);
    config.brand[15] = '\0';
    changed = true;
    DEBUG_PRINTF("[配置] 更新品牌: %s\n", config.brand);
  }

  if (doc.containsKey("model")) {
    config.model = doc["model"];
    changed = true;
    DEBUG_PRINTF("[配置] 更新型号: %d\n", config.model);
  }

  if (changed) {
    DEBUG_PRINTLN("[配置] ✅ 配置已更新");
    save(); // 自动保存
    return true;
  }

  return false;
}

DeviceConfig &ConfigManager::getConfig() { return config; }

void ConfigManager::printConfig() {
  DEBUG_PRINTLN("\n========== 设备配置 ==========");
  DEBUG_PRINTF("MQTT服务器: %s:%d\n", config.mqttServer, config.mqttPort);
  DEBUG_PRINTF("MQTT用户: %s\n", config.mqttUser);
  DEBUG_PRINTF("设备UUID: %s\n", config.deviceUUID);
  DEBUG_PRINTF("用户ID: %u\n", config.userId);
  DEBUG_PRINTF("传感器间隔: %u ms\n", config.sensorInterval);
  DEBUG_PRINTF("Ghost窗口: %u ms\n", config.ghostWindow);

  // ✅ 品牌配置
  if (config.brand[0] != '\0') {
    DEBUG_PRINTF("品牌: %s (型号: %d)\n", config.brand, config.model);
  } else {
    DEBUG_PRINTLN("品牌: 未配置");
  }

  DEBUG_PRINTLN("==============================\n");
}

uint8_t ConfigManager::calculateChecksum() {
  uint8_t sum = 0;
  uint8_t *ptr = (uint8_t *)&config;

  // 计算校验和（不包括checksum字段本身）
  for (size_t i = 0; i < sizeof(DeviceConfig) - 1; i++) {
    sum ^= ptr[i];
  }

  return sum;
}

bool ConfigManager::verifyChecksum() {
  uint8_t calculated = calculateChecksum();
  return calculated == config.checksum;
}

String ConfigManager::generateUUID() {
  // 使用MAC地址生成UUID
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "ESP_" + mac;
}
