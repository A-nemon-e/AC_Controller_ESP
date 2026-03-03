/*
 * I2C设备扫描工具
 * 用于检测I2C总线上的所有设备
 */

#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <Arduino.h>
#include <Wire.h>

class I2CScanner {
public:
  // 扫描I2C总线上的所有设备
  static void scan() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("  I2C设备扫描");
    Serial.println("========================================");

    uint8_t devicesFound = 0;

    Serial.println("扫描地址范围: 0x01-0x7F");
    Serial.println();

    for (uint8_t addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      uint8_t error = Wire.endTransmission();

      if (error == 0) {
        Serial.printf("✅ 发现设备: 0x%02X ", addr);

        // 识别已知设备
        if (addr == 0x38) {
          Serial.print("(AHT20温湿度传感器)");
        } else if (addr >= 0x50 && addr <= 0x55) {
          Serial.printf("(IS31FL3733 LED驱动 #%d)", addr - 0x50 + 1);
        }

        Serial.println();
        devicesFound++;
      }

      delay(10);
    }

    Serial.println();
    Serial.println("========================================");
    Serial.printf("  扫描完成，找到%d个设备\n", devicesFound);
    Serial.println("========================================");
    Serial.println();

    // 验证期望设备
    if (devicesFound >= 7) {
      Serial.println("✅ 所有设备在线（期望7个：1个AHT20 + 6个IS31FL3733）");
    } else {
      Serial.printf("⚠️  设备数量不符合预期！期望7个，实际%d个\n", devicesFound);
      Serial.println("   请检查硬件连接和I2C地址配置");
    }
  }

  // 检查特定地址是否在线
  static bool isDeviceOnline(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
  }

  // 批量检查LED芯片
  static bool checkLEDChips() {
    Serial.println("[I2C扫描] 检查6个IS31FL3733芯片...");

    bool allOnline = true;
    for (uint8_t i = 0; i < 6; i++) {
      uint8_t addr = 0x50 + i;
      bool online = isDeviceOnline(addr);

      Serial.printf("  芯片#%d (0x%02X): %s\n", i + 1, addr,
                    online ? "✅ 在线" : "❌ 离线");

      if (!online) {
        allOnline = false;
      }
    }

    return allOnline;
  }

  // 检查AHT20传感器
  static bool checkAHT20() {
    Serial.println("[I2C扫描] 检查AHT20传感器...");

    bool online = isDeviceOnline(0x38);
    Serial.printf("  AHT20 (0x38): %s\n", online ? "✅ 在线" : "❌ 离线");

    return online;
  }
};

#endif // I2C_SCANNER_H
