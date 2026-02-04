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
  // ✅ 强制刷新：先关闭再打开，确保 buffer 内容来自真实的 Flash
  EEPROM.end();
  EEPROM.begin(EEPROM_SIZE);

  DEBUG_PRINTLN("[配置] 从EEPROM加载配置...");

  // 读取配置结构体
  EEPROM.get(EEPROM_CONFIG_ADDR, config);

  uint32_t deviceId;
  EEPROM.get(EEPROM_DEVICE_ID, deviceId);

  // ⚠️ 关键修改：不要在校验前覆盖 config.userId，否则会导致校验失败且丢失数据
  // EEPROM.get(EEPROM_USER_ID, config.userId);

  // 验证校验和
  if (!verifyChecksum()) {
    DEBUG_PRINTF("[配置] ❌ 校验和错误! 计算值: 0x%02X, 存储值: 0x%02X\n",
                 calculateChecksum(), config.checksum);

    // 尝试从独立槽位恢复身份
    uint32_t savedUserId;
    EEPROM.get(EEPROM_USER_ID, savedUserId);
    DEBUG_PRINTF("[配置] 检查独立备份UserID: %u\n", savedUserId);

    resetToDefault();

    // 如果独立槽位有有效数据，恢复它
    if (savedUserId != 0 && savedUserId != 0xFFFFFFFF) {
      config.userId = savedUserId;
      DEBUG_PRINTF("[配置] 已从备份恢复userId: %u\n", savedUserId);

      save(); // 保存修复后的配置
      DEBUG_PRINTLN("[配置] ✅ 已通过备份修复配置");
      return true; // ✅ 视为加载成功，阻止 init() 再次重置
    }

    save();
    return false; // 只有真的无法恢复时才返回 false
  }

  DEBUG_PRINTF("[配置] 加载userId: %u\n", config.userId);

  // 额外验证：检查关键字段是否包含有效数据
  // 检查字符串是否为有效 ASCII（防止垃圾数据通过 checksum 验证）
  auto isValidString = [](const char *str, size_t len) -> bool {
    if (str[0] == '\0' || str[0] == (char)0xFF)
      return true; // 允许空字符串
    for (size_t i = 0; i < len && str[i] != '\0'; i++) {
      if (str[i] < 32 || str[i] > 126)
        return false; // 非可打印字符
    }
    return true;
  };

  // 验证所有字符串字段
  if (!isValidString(config.mqttServer, sizeof(config.mqttServer)) ||
      !isValidString(config.mqttUser, sizeof(config.mqttUser)) ||
      !isValidString(config.mqttPassword, sizeof(config.mqttPassword)) ||
      !isValidString(config.deviceUUID, sizeof(config.deviceUUID)) ||
      !isValidString(config.brand, sizeof(config.brand))) {
    DEBUG_PRINTLN("[配置] ❌ 包含无效字符，配置损坏");
    return false;
  }

  // 验证数值字段的合理性
  if (config.mqttPort == 0 || config.mqttPort > 65535) {
    DEBUG_PRINTLN("[配置] ❌ MQTT端口无效");
    return false;
  }

  DEBUG_PRINTLN("[配置] ✅ 配置加载成功");
  loaded = true;
  return true;
}

bool ConfigManager::save() {
  // ✅ 强制刷新
  EEPROM.end();
  EEPROM.begin(EEPROM_SIZE);

  DEBUG_PRINTLN("[配置] 保存配置到EEPROM...");

  // 计算校验和
  config.checksum = calculateChecksum();

  // 写入EEPROM
  EEPROM.put(EEPROM_CONFIG_ADDR, config);

  // ✅ 关键修复：同步保存 userId 到独立槽位，防止 load() 时被旧数据覆盖
  EEPROM.put(EEPROM_USER_ID, config.userId);

  bool success = EEPROM.commit();
  DEBUG_PRINTF("[配置] EEPROM提交结果: %s\n", success ? "成功" : "失败");

  DEBUG_PRINTLN("[配置] ✅ 配置已保存");
  DEBUG_PRINTF("[配置] 写入Checksum: 0x%02X, UserID: %u\n", config.checksum,
               config.userId);
  return success;
}

void ConfigManager::resetToDefault() {
  EEPROM.begin(EEPROM_SIZE); // ✅ 确保EEPROM已初始化
  DEBUG_PRINTLN("[配置] 重置为默认配置");

  // 清空整个结构体（防止残留数据）
  memset(&config, 0, sizeof(DeviceConfig));

  // MQTT服务器配置（使用config.h中的默认值）
  strncpy(config.mqttServer, MQTT_SERVER, sizeof(config.mqttServer) - 1);
  config.mqttServer[sizeof(config.mqttServer) - 1] = '\0'; // 确保空终止

  config.mqttPort = MQTT_PORT;
  DEBUG_PRINTF("[配置] 重置MQTT端口: %u (Hex: 0x%04X)\n", config.mqttPort,
               config.mqttPort);

  strncpy(config.mqttUser, MQTT_USER, sizeof(config.mqttUser) - 1);
  config.mqttUser[sizeof(config.mqttUser) - 1] = '\0';

  strncpy(config.mqttPassword, MQTT_PASSWORD, sizeof(config.mqttPassword) - 1);
  config.mqttPassword[sizeof(config.mqttPassword) - 1] = '\0';

  // 设备UUID：使用MAC地址生成
  String uuid = generateUUID();
  strncpy(config.deviceUUID, uuid.c_str(), sizeof(config.deviceUUID) - 1);
  config.deviceUUID[sizeof(config.deviceUUID) - 1] = '\0';

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

  // ✅ 关键修复：同时清除单独存储的userId和deviceId
  EEPROM.put(EEPROM_USER_ID, (uint32_t)0);
  EEPROM.put(EEPROM_DEVICE_ID, (uint32_t)0);
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

  // ✅ 新增：支持 deviceId 更新
  if (doc.containsKey("deviceId")) {
    uint32_t newId = doc["deviceId"];
    saveDeviceId(newId); // 直接保存到独立槽位
    DEBUG_PRINTF("[配置] 更新deviceId: %u\n", newId);
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

  // 调试：验证结构体对齐和关键数据
  // MQTT Port offset should be 64 (after 64 bytes of mqttServer)
  // 1883 = 0x075B. Low byte 0x5B, High byte 0x07.
  DEBUG_PRINTF("[校验] StructSize: %d, PortBytes: 0x%02X 0x%02X\n",
               sizeof(DeviceConfig), ptr[64], ptr[65]);

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

// ✅ 新增：保存userId到EEPROM
void ConfigManager::saveUserId(uint32_t userId) {
  EEPROM.begin(EEPROM_SIZE); // ✅ 确保EEPROM已初始化
  config.userId = userId;
  EEPROM.put(EEPROM_USER_ID, userId);
  EEPROM.commit();
  DEBUG_PRINTF("[配置] ✅ 已保存userId: %u\n", userId);
}

// ✅ 新增：保存deviceId到EEPROM
void ConfigManager::saveDeviceId(uint32_t deviceId) {
  EEPROM.begin(EEPROM_SIZE); // ✅ 确保EEPROM已初始化
  EEPROM.put(EEPROM_DEVICE_ID, deviceId);
  EEPROM.commit();
  DEBUG_PRINTF("[配置] ✅ 已保存deviceId: %u\n", deviceId);
}
