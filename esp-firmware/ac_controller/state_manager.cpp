/*
 * 空调状态管理器 - 实现
 */

#include "state_manager.h"
#include "mqtt_client.h"
#include "sensors.h"
#include <ArduinoJson.h>

// 静态成员初始化
AirConditionerState StateManager::currentState = {
    false,  // power
    "cool", // mode
    26,     // temp
    0,      // fan (auto)
    false,  // swingV
    false,  // swingH
    "init", // source
    0       // lastUpdate
};
bool StateManager::stateChanged = false;

void StateManager::init() {
  DEBUG_PRINTLN("[状态] 初始化状态管理器");

  // 尝试从EEPROM加载状态
  if (load()) {
    DEBUG_PRINTLN("[状态] ✅ 从EEPROM加载状态");
  } else {
    DEBUG_PRINTLN("[状态] 使用默认状态");
  }
}

void StateManager::setState(bool power, const char *mode, uint8_t temp,
                            uint8_t fan, bool swingV, bool swingH,
                            const char *source) {
  currentState.power = power;
  currentState.mode = String(mode);
  currentState.temp = constrain(temp, 16, 30);
  currentState.fan = constrain(fan, 0, 3);
  currentState.swingV = swingV;
  currentState.swingH = swingH;
  currentState.source = String(source);
  currentState.lastUpdate = millis();

  stateChanged = true;

  DEBUG_PRINTLN("[状态] 状态已更新");
  DEBUG_PRINTF("[状态] 电源: %s, 模式: %s, 温度: %d°C\n", power ? "开" : "关",
               mode, temp);

  // 自动发布状态
  publishState();

  // 可选：保存到EEPROM
  // save();
}

bool StateManager::updateFromJSON(const char *json) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINTLN("[状态] ❌ JSON解析失败");
    return false;
  }

  // 更新状态（仅更新存在的字段）
  if (doc.containsKey("power"))
    currentState.power = doc["power"];
  if (doc.containsKey("mode"))
    currentState.mode = doc["mode"].as<String>();
  if (doc.containsKey("temp"))
    currentState.temp = constrain((int)doc["temp"], 16, 30);
  if (doc.containsKey("fan"))
    currentState.fan = constrain((int)doc["fan"], 0, 3);
  if (doc.containsKey("swingVertical"))
    currentState.swingV = doc["swingVertical"];
  if (doc.containsKey("swingHorizontal"))
    currentState.swingH = doc["swingHorizontal"];
  if (doc.containsKey("source"))
    currentState.source = doc["source"].as<String>();

  currentState.lastUpdate = millis();
  stateChanged = true;

  DEBUG_PRINTLN("[状态] ✅ 从JSON更新状态");
  return true;
}

AirConditionerState &StateManager::getState() { return currentState; }

void StateManager::publishState() {
  if (!MQTTClient::isConnected())
    return;

  // 构建状态JSON（包含传感器数据）
  StaticJsonDocument<512> doc;

  // 空调状态
  doc["power"] = currentState.power;
  doc["mode"] = currentState.mode;
  doc["targetTemp"] = currentState.temp;
  doc["fan"] = currentState.fan;
  doc["swingVertical"] = currentState.swingV;
  doc["swingHorizontal"] = currentState.swingH;
  doc["source"] = currentState.source;

  // 传感器数据
  doc["temp"] = Sensors::getTemperature();
  doc["hum"] = Sensors::getHumidity();
  doc["current"] = Sensors::getCurrent();

  doc["timestamp"] = millis() / 1000;

  char payload[512];
  serializeJson(doc, payload);

  // 发布到MQTT
  String topic = MQTTClient::getTopic("status");
  if (MQTTClient::publish(topic.c_str(), payload)) {
    DEBUG_PRINTLN("[状态] ✅ 状态已发布");
    stateChanged = false;
  }
}

void StateManager::save() {
  // TODO: 实现EEPROM保存
  DEBUG_PRINTLN("[状态] EEPROM保存功能待实现");
}

bool StateManager::load() {
  // TODO: 实现EEPROM加载
  return false;
}
