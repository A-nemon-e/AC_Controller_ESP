/*
 * 传感器模块
 *
 * 功能：
 * - AHT20温湿度传感器读取（I2C）
 * - ADC电流采样
 * - 定期上报MQTT
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include <Adafruit_AHTX0.h>
#include <Arduino.h>
#include <Wire.h>


class Sensors {
public:
  // 初始化传感器
  static bool init();

  // 更新传感器读数（在loop中定期调用）
  static void update();

  // 获取当前读数
  static float getTemperature();
  static float getHumidity();
  static float getCurrent();

  // 强制读取一次
  static bool read();

  // 上报到MQTT
  static void publishStatus();

private:
  static Adafruit_AHTX0 aht;
  static float temperature;
  static float humidity;
  static float current;
  static unsigned long lastReadTime;
  static bool initialized;

  // ADC电流采样
  static float readCurrent();
};

#endif // SENSORS_H
