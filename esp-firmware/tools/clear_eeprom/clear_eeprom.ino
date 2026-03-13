/*
 * EEPROM清除工具
 * 用于清除ESP8266的EEPROM，解决WiFi配置问题
 */

#include <EEPROM.h>

#define EEPROM_SIZE 4096

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("  EEPROM清除工具");
  Serial.println("========================================\n");
  
  // 初始化EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // 显示当前EEPROM内容（前128字节）
  Serial.println("当前EEPROM内容（前128字节）：");
  for (int i = 0; i < 128; i++) {
    if (i % 16 == 0) {
      Serial.printf("\n%04X: ", i);
    }
    uint8_t val = EEPROM.read(i);
    Serial.printf("%02X ", val);
  }
  Serial.println("\n");
  
  // 显示SSID区域（0-31）
  Serial.println("SSID区域（地址0-31）：");
  for (int i = 0; i < 32; i++) {
    char c = EEPROM.read(i);
    if (c >= 32 && c < 127) {
      Serial.print(c);
    } else {
      Serial.printf("[%02X]", (uint8_t)c);
    }
  }
  Serial.println("\n");
  
  // 清除EEPROM
  Serial.println("正在清除EEPROM...");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
    if (i % 256 == 0) {
      Serial.print(".");
    }
  }
  Serial.println("\nEEPROM已清除！");
  
  // 提交更改
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("\n✅ EEPROM清除完成！");
  Serial.println("请重新上传主程序并配置WiFi。");
  Serial.println("========================================\n");
}

void loop() {
  // 什么都不做
}
