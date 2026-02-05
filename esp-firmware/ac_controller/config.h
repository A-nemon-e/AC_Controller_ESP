/*
 * ESP12F空调控制器 - 配置文件
 *
 * 此文件包含所有硬件引脚定义和可配置参数
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// ===== 硬件引脚定义 =====
#define PIN_IR_SEND 14 // D5 - 红外发射LED
#define PIN_IR_RECV 13 // D7 - 1838B红外接收头
#define PIN_MIC 12     // D6 - 麦克风数字输出
#define PIN_SDA 4      // D2 - I2C数据线（AHT20）
#define PIN_SCL 5      // D1 - I2C时钟线（AHT20）
#define PIN_LED_SYS 15 // D8 - 系统状态LED
#define PIN_LED_IR 16  // D0 - 红外接收指示LED
#define PIN_ADC A0     // A0 - 电流互感器

// ===== WiFi配置（默认值，可通过SmartConfig修改）=====
// 注意：实际的WiFi凭证会存储在EEPROM中
// ⚠️ 测试环境：直接硬编码WiFi信息
#define WIFI_SSID "TP-LINK_AFC5F2" // ✅ 硬编码WiFi名称
#define WIFI_PASSWORD "2002051377" // ✅ 硬编码WiFi密码

#define WIFI_CONNECT_TIMEOUT 20000 // WiFi连接超时（毫秒）
#define WIFI_RECONNECT_DELAY 5000  // 重连延迟（毫秒）

// ===== MQTT配置 =====
// TODO: 改进为从EEPROM读取，支持服务器下发配置
// ⚠️ 暂时未部署MQTT服务器，保持默认值即可
#define MQTT_SERVER "10.0.10.13" // MQTT服务器地址（暂未使用）
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASSWORD "2307yU5*"

// 设备标识（临时硬编码，TODO: 改为从EEPROM或MAC生成）
#define DEVICE_UUID "esp_001"
#define USER_ID 0

// MQTT连接参数
#define MQTT_KEEPALIVE 60         // 心跳间隔（秒）
#define MQTT_RECONNECT_DELAY 5000 // 重连延迟（毫秒）
#define MQTT_BUFFER_SIZE 2048     // MQTT消息缓冲区大小 (由512扩容，适配长消息)

// ===== 定时器配置默认值 =====
#define DEFAULT_SENSOR_INTERVAL 30000
#define DEFAULT_HEARTBEAT_INTERVAL 60000
#define DEFAULT_GHOST_WINDOW 30000

// ===== 红外配置 =====
#define IR_RECV_BUFFER_SIZE 1024  // 红外接收缓冲区
#define IR_RECV_TIMEOUT 50        // 接收超时（毫秒）
#define IR_CARRIER_FREQ 38        // 载波频率（kHz）
#define IR_LEARNING_TIMEOUT 30000 // 学习模式超时（30秒）

// ===== 传感器配置 =====
#define ADC_SAMPLES 10     // ADC采样次数
#define CURRENT_OFFSET 512 // 电流传感器零点偏移
#define CURRENT_RATIO 0.01 // 电流转换比例

// ===== EEPROM存储地址 =====
#define EEPROM_SIZE 4096       // ✅ 扩容到4KB (ESP8266 Flash支持)
#define EEPROM_WIFI_SSID 0     // SSID起始地址（最多32字节）
#define EEPROM_WIFI_PASS 32    // 密码起始地址（最多64字节）
#define EEPROM_DEVICE_UUID 96  // UUID起始地址（最多32字节）
#define EEPROM_CONFIG_FLAG 128 // 配置标志位（1字节，0x55表示已配置）
#define EEPROM_USER_ID 129     // ✅ 用户ID地址（4字节）
#define EEPROM_DEVICE_ID 133   // ✅ 设备ID地址（4字节）
#define EEPROM_SCENES_ADDR 512 // ✅ 学习场景起始地址（512字节空间）

// ===== 调试配置 =====
#define SERIAL_BAUD 115200
#define DEBUG_ENABLED true // 设为false关闭调试输出

// 调试宏
#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
