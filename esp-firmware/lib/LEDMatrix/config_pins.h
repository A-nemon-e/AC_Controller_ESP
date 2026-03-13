/**
 * config_pins.h - 硬件引脚定义
 * 
 * ESP-12F GPIO完整分配
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// ============================================
// I2C总线
// ============================================
#define PIN_SDA 4                   // GPIO4 - I2C数据线
#define PIN_SCL 5                   // GPIO5 - I2C时钟线
#define I2C_CLOCK_SPEED 400000      // 400kHz快速模式

// ============================================
// IS31FL3733控制
// ============================================
#define PIN_SDB 0                   // GPIO0 - Shutdown控制（低电平复位）

// ============================================
// 用户输入
// ============================================
#define PIN_BUTTON 16               // GPIO16 - 微动开关（高电平有效，内部下拉）

// ============================================
// 系统指示
// ============================================
#define PIN_LED_SYS 15              // GPIO15 - 系统状态LED

// ============================================
// 红外系统（复用现有ac_controller）
// ============================================
#define PIN_IR_SEND 14              // GPIO14 - 红外发射（SS8050驱动）
#define PIN_IR_RECV 2               // GPIO2 - 红外接收（1838B）

// ============================================
// 传感器
// ============================================
#define PIN_AHT_SDA PIN_SDA         // AHT20共享I2C总线
#define PIN_AHT_SCL PIN_SCL

// ============================================
// 预留扩展（3收1发）- 暂缓开发
// ============================================
// #define PIN_IR_RECV_2 12         // GPIO12 - 预留红外接收2
// #define PIN_IR_RECV_3 13         // GPIO13 - 预留红外接收3

// ============================================
// 屏幕参数
// ============================================
#define DISPLAY_WIDTH 42
#define DISPLAY_HEIGHT 11
#define NUM_CHIPS 6
#define CHIP_COLS 7

// ============================================
// LED矩阵参数
// ============================================
#define SCREEN_COLS DISPLAY_WIDTH   // 42列
#define SCREEN_ROWS DISPLAY_HEIGHT  // 11行
#define SCREEN_PIXELS (SCREEN_COLS * SCREEN_ROWS)  // 462像素

// ============================================
// I2C设备地址
// ============================================
#define AHT20_ADDRESS 0x38          // AHT20温湿度传感器

// IS31FL3733地址（根据ADDR1/ADDR2引脚配置）
#define CHIP0_ADDR 0x55             // chip0: ADDR1=SCL, ADDR2=SCL (列35-41)
#define CHIP1_ADDR 0x54             // chip1: ADDR1=GND, ADDR2=SCL (列28-34)
#define CHIP2_ADDR 0x53             // chip2: ADDR1=VCC, ADDR2=GND (列21-27)
#define CHIP3_ADDR 0x52             // chip3: ADDR1=SDA, ADDR2=GND (列14-20)
#define CHIP4_ADDR 0x51             // chip4: ADDR1=SCL, ADDR2=GND (列7-13)
#define CHIP5_ADDR 0x50             // chip5: ADDR1=GND, ADDR2=GND (列0-6)

// ============================================
// EEPROM布局（与DisplayConfig配合使用）
// ============================================
// ⚠️ 重要：避开 WiFiManager 使用的区域 (0x00-0x5F)
// WiFiManager: 0x00-0x1F (SSID, 32 bytes), 0x20-0x5F (Password, 64 bytes)
#ifndef EEPROM_SIZE
  #define EEPROM_SIZE 512             // EEPROM总大小（如果未定义）
#endif

// DisplayConfig 使用 0x100 开始的区域（256 bytes起）
#define EEPROM_MAGIC_ADDR 0x100      // 魔数 (4 bytes)
#define EEPROM_VERSION_ADDR 0x104    // 版本 (2 bytes)
#define EEPROM_CHECKSUM_ADDR 0x106   // 校验和 (2 bytes)
#define EEPROM_DISPLAY_SETTINGS_ADDR 0x110  // DisplaySettings (32 bytes)
#define EEPROM_CARD_CONFIG_ADDR 0x130       // 卡片配置预留 (32 bytes)
#define EEPROM_USER_DATA_ADDR 0x150         // 用户数据预留 (176 bytes)

// 魔数和版本
#define EEPROM_MAGIC 0x44495350     // "DISP" in ASCII
#define EEPROM_VERSION 1

#endif // CONFIG_PINS_H
