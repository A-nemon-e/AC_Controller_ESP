/*
 * 传感器模块 - 实现
 */

#include "sensors.h"
#include "config_manager.h"
#include "mqtt_client.h"
#include "state_manager.h" // ✅ 新增：需要访问 StateManager
#include <ArduinoJson.h>

// 静态成员初始化
Adafruit_AHTX0 Sensors::aht;
float Sensors::temperature = 0.0;
float Sensors::humidity = 0.0;
float Sensors::current = 0.0;
unsigned long Sensors::lastReadTime = 0;
bool Sensors::initialized = false;

bool Sensors::init() {
  DEBUG_PRINTLN("[传感器] 初始化传感器模块");

  // 初始化I2C
  Wire.begin(PIN_SDA, PIN_SCL);

  // 初始化AHT20
  if (!aht.begin()) {
    DEBUG_PRINTLN("[传感器] ❌ AHT20初始化失败");
    return false;
  }

  DEBUG_PRINTLN("[传感器] ✅ AHT20初始化成功");

  // 首次读取
  if (read()) {
    initialized = true;
    DEBUG_PRINTLN("[传感器] ✅ 传感器模块就绪");
    return true;
  }

  return false;
}

void Sensors::update() {
  if (!initialized)
    return;

  unsigned long now = millis();
  DeviceConfig &cfg = ConfigManager::getConfig();

  // 检查是否到达上报间隔
  if (now - lastReadTime >= cfg.sensorInterval) {
    if (read()) {
      publishStatus();
      lastReadTime = now;
    }
  }
}

float Sensors::getTemperature() { return temperature; }

float Sensors::getHumidity() { return humidity; }

float Sensors::getCurrent() { return current; }

bool Sensors::read() {
  // 读取AHT20
  sensors_event_t humidity_event, temp_event;
  aht.getEvent(&humidity_event, &temp_event);

  temperature = temp_event.temperature;
  humidity = humidity_event.relative_humidity;

  // 读取电流
  current = readCurrent();

  DEBUG_PRINTF("[传感器] 温度: %.1f°C, 湿度: %.1f%%, 电流: %.2fA\n",
               temperature, humidity, current);

  return true;
}

void Sensors::publishStatus() {
  if (!MQTTClient::isConnected()) {
    DEBUG_PRINTLN("[传感器] MQTT未连接，跳过上报");
    return;
  }

  // ✅ 修复：获取完整的空调状态，合并传感器数据
  // 防止只发送温湿度导致后端丢失空调控制状态
  AirConditionerState &acState = StateManager::getState();

  StaticJsonDocument<512> doc;

  // 1. 填入空调控制状态
  doc["power"] = acState.power;
  doc["mode"] = acState.mode;
  // doc["targetTemp"] = acState.temp; // ❌ 移除重复
  doc["setTemp"] = acState.temp; // ✅ 保留标准字段
  doc["fan"] = acState.fan;
  doc["swingVertical"] = acState.swingV;
  doc["swingHorizontal"] = acState.swingH;
  doc["source"] = "sensor_report";

  // 2. 填入传感器数据
  doc["temp"] = temperature; // 环境温度
  doc["hum"] = humidity;     // 环境湿度
  doc["current"] = current;  // 电流

  doc["timestamp"] = millis() / 1000;

  char payload[512];
  serializeJson(doc, payload);

  // 发布到MQTT
  String topic = MQTTClient::getTopic("status");
  if (MQTTClient::publish(topic.c_str(), payload)) {
    DEBUG_PRINTLN("[传感器] ✅ 完整状态已上报");
  } else {
    DEBUG_PRINTLN("[传感器] ❌ 状态上报失败");
  }
}

float Sensors::readCurrent() {
  // ADC采样（多次平均）
  uint32_t adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(PIN_ADC);
    delay(5);
  }

  float adcValue = adcSum / (float)ADC_SAMPLES;

  // ✅ 打印ADC原始值（用于调试和参数调整）
  DEBUG_PRINTF("[传感器] ADC原始值: %.2f (范围0-1023)\n", adcValue);

  // 转换为电流值
  // 公式：I = (ADC * 3.3 / 1024 - offset) * ratio
  float voltage = (adcValue * 3.3) / 1024.0;
  float offsetVoltage = (CURRENT_OFFSET * 3.3) / 1024.0;
  float currentValue = (voltage - offsetVoltage) / CURRENT_RATIO;

  // 限制范围（避免负值）
  if (currentValue < 0)
    currentValue = 0;

  // ✅ 打印计算后的电流值和中间电压
  DEBUG_PRINTF("[传感器] 计算电流: %.3fA (电压: %.3fV, 偏移: %.3fV)\n",
               currentValue, voltage, offsetVoltage);

  return currentValue;
}
