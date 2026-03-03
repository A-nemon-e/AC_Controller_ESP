/*
 * IS31FL3733 x6 演示固件
 * 基于 ESP-12F (ESP8266)，驱动 42x11 LED 屏幕
 *
 * 引脚配置:
 *   - I2C SDA: IO4
 *   - I2C SCL: IO5
 *   - SDB: IO0
 *   - 按钮: IO16 (内部下拉，高电平有效)
 *
 * 按钮功能摘要:
 *   - 短按: 亮屏/息屏
 *   - 长按: 调节显示亮度
 *   - 双击: 切换显示模式 (扫描、呼吸、动画、时间、温湿度等)
 */

#include "is31fl3733.hpp"
#include <Adafruit_AHTX0.h>
#include <ESP8266WiFi.h>
#include <EasyButton.h>
#include <Wire.h>
#include <time.h>
using namespace IS31FL3733;
Adafruit_AHTX0 aht;
bool ahtReady = false;
const char *WIFI_SSID = "TP-LINK_AFC5F2";
const char *WIFI_PASS = "2002051377";
bool ntpSynced = false;
uint32_t ntpLastSyncMs = 0;
#define NTP_RESYNC_MS (24UL * 3600UL * 1000UL)
#define PIN_SDA 4
#define PIN_SCL 5
#define PIN_SDB 0
#define PIN_BUTTON 16
#define NUM_CHIPS 6
#define CHIP_COLS 7
#define SCREEN_ROWS 11
#define SCREEN_COLS 42
#define GCC_DEFAULT 0xFF
#define SCAN_DELAY_MS 60
#define ANIM_MS 18

uint8_t currentMode = 0;
// Forward declarations
void processButton();
void smartDelay(uint32_t ms);
static const uint8_t FONT5x7[][7] PROGMEM = {
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},
    {0x04, 0x0A, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x13, 0x11, 0x11, 0x0F},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04, 0x04},
    {0x11, 0x11, 0x15, 0x15, 0x15, 0x1B, 0x11},
    {0x11, 0x0A, 0x04, 0x04, 0x04, 0x0A, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
};
static const uint8_t FONT3x5[][5] PROGMEM = {
    // Numbers 0-9
    {0x06, 0x05, 0x05, 0x05,
     0x03}, // 0 (Added top-left bit2 of row0, bottom-right bit0 of row4 -
            // 010->110, 010->011)
    {0x02, 0x06, 0x02, 0x02, 0x07}, // 1
    {0x06, 0x01, 0x03, 0x04, 0x07}, // 2
    {0x06, 0x01, 0x03, 0x01, 0x06}, // 3
    {0x01, 0x03, 0x05, 0x07, 0x01}, // 4
    {0x07, 0x04, 0x07, 0x01, 0x06}, // 5
    {0x03, 0x04, 0x07, 0x05, 0x03}, // 6
    {0x07, 0x01, 0x02, 0x02, 0x02}, // 7
    {0x03, 0x05, 0x02, 0x05, 0x07}, // 8
    {0x03, 0x05, 0x07, 0x01, 0x02}, // 9

    // Letters A-Z
    {0x02, 0x05, 0x07, 0x05, 0x05}, // A
    {0x06, 0x05, 0x06, 0x05, 0x06}, // B
    {0x03, 0x05, 0x04, 0x05,
     0x06}, // C (remove top-left, add pixel below top-right, add bottom-left,
            // move bottom-right up one)
    {0x06, 0x05, 0x05, 0x05, 0x06}, // D
    {0x07, 0x04, 0x06, 0x04, 0x07}, // E
    {0x07, 0x04, 0x06, 0x04, 0x04}, // F
    {0x03, 0x04, 0x05, 0x05, 0x03}, // G
    {0x05, 0x05, 0x07, 0x05, 0x05}, // H
    {0x07, 0x02, 0x02, 0x02, 0x07}, // I
    {0x03, 0x01, 0x01, 0x05, 0x02}, // J
    {0x05, 0x06, 0x06, 0x05, 0x05}, // K
    {0x04, 0x04, 0x04, 0x04, 0x07}, // L
    {0x05, 0x07, 0x05, 0x05, 0x05}, // M
    {0x06, 0x05, 0x05, 0x05, 0x05}, // N
    {0x02, 0x05, 0x05, 0x05, 0x02}, // O
    {0x06, 0x05, 0x07, 0x04, 0x04}, // P
    {0x02, 0x05, 0x05, 0x07, 0x03}, // Q
    {0x06, 0x05, 0x07, 0x06, 0x05}, // R
    {0x03, 0x04, 0x02, 0x01, 0x06}, // S
    {0x07, 0x02, 0x02, 0x02, 0x02}, // T
    {0x05, 0x05, 0x05, 0x05, 0x03}, // U
    {0x05, 0x05, 0x05, 0x05, 0x02}, // V
    {0x05, 0x05, 0x05, 0x07, 0x05}, // W
    {0x05, 0x05, 0x02, 0x05, 0x05}, // X
    {0x05, 0x05, 0x02, 0x02, 0x02}, // Y
    {0x07, 0x01, 0x02, 0x04, 0x07}, // Z
};
static const uint8_t FONT6x9[][9] PROGMEM = {
    {0x1E, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E},
    {0x0C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3E},
    {0x1E, 0x33, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x3F},
    {0x1E, 0x33, 0x03, 0x0E, 0x03, 0x03, 0x03, 0x33, 0x1E},
    {0x06, 0x0E, 0x1E, 0x36, 0x36, 0x3F, 0x06, 0x06, 0x06},
    {0x3F, 0x30, 0x30, 0x3E, 0x03, 0x03, 0x03, 0x33, 0x1E},
    {0x1E, 0x33, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x33, 0x1E},
    {0x3F, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18},
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x33, 0x33, 0x1E},
    {0x1E, 0x33, 0x33, 0x1F, 0x03, 0x03, 0x03, 0x33, 0x1E},
};

// 3x9 Font (Numbers 0-9 and uppercase A-Z)
static const uint8_t FONT3x9[][9] PROGMEM = {
    // Numbers 0-9
    {0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x02}, // 0
    {0x02, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07}, // 1
    {0x06, 0x01, 0x01, 0x01, 0x03, 0x04, 0x04, 0x04, 0x07}, // 2
    {0x06, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x06}, // 3
    {0x05, 0x05, 0x05, 0x05, 0x07, 0x01, 0x01, 0x01, 0x01}, // 4
    {0x07, 0x04, 0x04, 0x06, 0x01, 0x01, 0x01, 0x05, 0x06}, // 5
    {0x06, 0x05, 0x04, 0x04, 0x06, 0x05, 0x05, 0x05, 0x02}, // 6
    {0x07, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02}, // 7
    {0x03, 0x05, 0x05, 0x05, 0x02, 0x05, 0x05, 0x05, 0x07}, // 8
    {0x02, 0x05, 0x05, 0x05, 0x03, 0x01, 0x01, 0x05, 0x03}, // 9

    // Letters A-Z (Extrapolated from the image style "The Quick Brown Fox...")
    {0x02, 0x05, 0x05, 0x05, 0x07, 0x05, 0x05, 0x05, 0x05}, // A
    {0x06, 0x05, 0x05, 0x05, 0x06, 0x05, 0x05, 0x05, 0x06}, // B
    {0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03}, // C
    {0x06, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06}, // D
    {0x07, 0x04, 0x04, 0x04, 0x06, 0x04, 0x04, 0x04, 0x07}, // E
    {0x07, 0x04, 0x04, 0x04, 0x06, 0x04, 0x04, 0x04, 0x04}, // F
    {0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x03}, // G
    {0x05, 0x05, 0x05, 0x05, 0x07, 0x05, 0x05, 0x05, 0x05}, // H
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07}, // I
    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x05, 0x02}, // J
    {0x05, 0x05, 0x05, 0x06, 0x04, 0x06, 0x05, 0x05, 0x05}, // K
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07}, // L
    {0x05, 0x07, 0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05}, // M
    {0x06, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
     0x05}, // N (Matches lowercase n style from image if needed, here
            // uppercase)
    {0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x02}, // O
    {0x06, 0x05, 0x05, 0x05, 0x06, 0x04, 0x04, 0x04, 0x04}, // P
    {0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x03, 0x01}, // Q
    {0x06, 0x05, 0x05, 0x05, 0x06, 0x05, 0x05, 0x05, 0x05}, // R
    {0x03, 0x04, 0x04, 0x04, 0x02, 0x01, 0x01, 0x01, 0x06}, // S
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}, // T
    {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x02}, // U
    {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x02, 0x02}, // V
    {0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x07, 0x05, 0x05}, // W
    {0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x05, 0x05, 0x05}, // X
    {0x05, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02}, // Y
    {0x07, 0x01, 0x01, 0x02, 0x02, 0x02, 0x04, 0x04, 0x07}, // Z
};

// 5x5 Font
static const uint8_t FONT5x5[][5] PROGMEM = {
    // Numbers 0-9
    {0x0E, 0x11, 0x15, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x01, 0x06, 0x08, 0x1F}, // 2
    {0x1F, 0x02, 0x04, 0x01, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x1F, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x1E}, // 5
    {0x06, 0x08, 0x1E, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x04}, // 7
    {0x0E, 0x11, 0x0E, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x0F, 0x02, 0x0C}, // 9

    // Letters A-Z
    {0x0E, 0x11, 0x1F, 0x11, 0x11}, // A
    {0x1E, 0x11, 0x1E, 0x11, 0x1E}, // B
    {0x0E, 0x11, 0x10, 0x11, 0x0E}, // C
    {0x1C, 0x12, 0x11, 0x12, 0x1C}, // D
    {0x1F, 0x10, 0x1E, 0x10, 0x1F}, // E
    {0x1F, 0x10, 0x1E, 0x10, 0x10}, // F
    {0x0E, 0x11, 0x13, 0x11, 0x0F}, // G
    {0x11, 0x11, 0x1F, 0x11, 0x11}, // H
    {0x0E, 0x04, 0x04, 0x04, 0x0E}, // I
    {0x07, 0x02, 0x02, 0x12, 0x0C}, // J
    {0x11, 0x12, 0x1C, 0x12, 0x11}, // K
    {0x10, 0x10, 0x10, 0x10, 0x1F}, // L
    {0x11, 0x1B, 0x15, 0x11, 0x11}, // M
    {0x11, 0x19, 0x15, 0x13, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x1E, 0x10, 0x10}, // P
    {0x0E, 0x11, 0x15, 0x12, 0x0D}, // Q
    {0x1E, 0x11, 0x1E, 0x14, 0x12}, // R
    {0x0E, 0x10, 0x0E, 0x01, 0x0E}, // S
    {0x1F, 0x04, 0x04, 0x04, 0x04}, // T
    {0x11, 0x11, 0x11, 0x11, 0x0E}, // U
    {0x11, 0x11, 0x11, 0x0A, 0x04}, // V
    {0x11, 0x11, 0x15, 0x15, 0x0A}, // W
    {0x11, 0x0A, 0x04, 0x0A, 0x11}, // X
    {0x11, 0x11, 0x0A, 0x04, 0x04}, // Y
    {0x1F, 0x02, 0x04, 0x08, 0x1F}, // Z
};
static const uint8_t SYMBOL_DOT[5] PROGMEM = {0x00, 0x00, 0x00, 0x03, 0x03};
static const uint8_t SYMBOL_PCT[5] PROGMEM = {0x05, 0x01, 0x02, 0x04, 0x05};
static const uint8_t SYMBOL_CELSIUS[5] PROGMEM = {0x04, 0x0A, 0x05, 0x01, 0x02};
static const uint8_t ICON_SUN[7] PROGMEM = {0x08, 0x2A, 0x1C, 0x77,
                                            0x1C, 0x2A, 0x08};
static const uint8_t ICON_RAIN[7] PROGMEM = {0x00, 0x1C, 0x7F, 0x7F,
                                             0x08, 0x08, 0x08};
static const uint8_t ICON_CLOUD[7] PROGMEM = {0x00, 0x00, 0x0E, 0x1F,
                                              0x7F, 0x7F, 0x7F};
static const uint8_t ICON_SNOW[7] PROGMEM = {0x08, 0x6B, 0x3E, 0x08,
                                             0x3E, 0x6B, 0x08};
uint8_t i2c_write_reg(const uint8_t addr, const uint8_t reg, const uint8_t *buf,
                      const uint8_t cnt) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t n = Wire.write(buf, cnt);
  Wire.endTransmission();
  return n;
}
uint8_t i2c_read_reg(const uint8_t addr, const uint8_t reg, uint8_t *buf,
                     const uint8_t cnt) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  uint8_t n = Wire.requestFrom(addr, cnt);
  for (uint8_t i = 0; i < n && i < cnt; i++)
    buf[i] = Wire.read();
  return n;
}
IS31FL3733Driver drv0(ADDR::GND, ADDR::GND, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver drv1(ADDR::SCL, ADDR::GND, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver drv2(ADDR::SDA, ADDR::GND, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver drv3(ADDR::VCC, ADDR::GND, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver drv4(ADDR::GND, ADDR::SCL, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver drv5(ADDR::SCL, ADDR::SCL, &i2c_read_reg, &i2c_write_reg);
IS31FL3733Driver *drivers[NUM_CHIPS] = {&drv0, &drv1, &drv2,
                                        &drv3, &drv4, &drv5};
uint8_t gccValue = 120;
bool screenOn = true;
int8_t brtDir = -1;
void applyBrightness() {
  uint8_t val = screenOn ? gccValue : 0;
  for (int i = 0; i < NUM_CHIPS; i++)
    drivers[i]->SetGCC(val);
}
uint8_t fb[SCREEN_ROWS][SCREEN_COLS];
void fbClear() { memset(fb, 0, sizeof(fb)); }
void fbSet(int x, int y, uint8_t v) {
  if (x >= 0 && x < SCREEN_COLS && y >= 0 && y < SCREEN_ROWS)
    fb[y][x] = v;
}
void fbRender() {
  static uint8_t pwmBuf[192];
  for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
    memset(pwmBuf, 0, 192);
    for (int8_t sw = CHIP_COLS - 1; sw >= 0; sw--) {
      uint8_t physX = chip * CHIP_COLS + (CHIP_COLS - 1 - sw);
      for (uint8_t cs = 0; cs < SCREEN_ROWS; cs++)
        pwmBuf[sw * 16 + cs] = fb[cs][physX];
    }
    drivers[chip]->SetPWM(pwmBuf);
  }
}
void drawChar(int x, int y, char c, uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 7; r++) {
    uint8_t row = pgm_read_byte(&FONT5x7[idx][r]);
    for (int col = 0; col < 5; col++)
      if (row & (1 << (4 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawStr(int x, int y, const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    drawChar(x, y, *s++, bri);
    x += 6;
  }
}
int strW(const char *s) {
  int n = strlen(s);
  return n > 0 ? n * 6 - 1 : 0;
}
int centerX(const char *s) { return (SCREEN_COLS - strW(s)) / 2; }
void drawChar3x9(int x, int y, char c, uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 9; r++) {
    uint8_t row = pgm_read_byte(&FONT3x9[idx][r]);
    for (int col = 0; col < 3; col++)
      if (row & (1 << (2 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawStr3x9(int x, int y, const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    drawChar3x9(x, y, *s++, bri);
    x += 4;
  }
}
int strW3x9(const char *s) {
  int n = strlen(s);
  return n > 0 ? n * 4 - 1 : 0;
}
int centerX3x9(const char *s) { return (SCREEN_COLS - strW3x9(s)) / 2; }

void drawChar5x5(int x, int y, char c, uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 5; r++) {
    uint8_t row = pgm_read_byte(&FONT5x5[idx][r]);
    for (int col = 0; col < 5; col++)
      if (row & (1 << (4 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawStr5x5(int x, int y, const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    drawChar5x5(x, y, *s++, bri);
    x += 6;
  }
}
// int strW5x5(const char *s) {
//   int n = strlen(s);
//   return n > 0 ? n * 6 - 1 : 0;
// }
int centerX5x5(const char *s) { return (SCREEN_COLS - strW5x5(s)) / 2; }

void drawChar3x9Buffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                       char c, uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 9; r++) {
    uint8_t row = pgm_read_byte(&FONT3x9[idx][r]);
    for (int col = 0; col < 3; col++) {
      if (row & (1 << (2 - col))) {
        if (x + col >= 0 && x + col < SCREEN_COLS && y + r >= 0 &&
            y + r < SCREEN_ROWS)
          buf[y + r][x + col] = bri;
      }
    }
  }
}
void drawStr3x9Buffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                      const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    drawChar3x9Buffer(buf, x, y, *s++, bri);
    x += 4;
  }
}

void drawCharSmall(int x, int y, char c, uint8_t bri = 0xFF) {
  if (c == ':') {
    fbSet(x + 1, y + 1, bri);
    fbSet(x + 1, y + 3, bri);
    return;
  } else if (c == '.') {
    fbSet(x + 1, y + 4, bri);
    return;
  }
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 5; r++) {
    uint8_t row = pgm_read_byte(&FONT3x5[idx][r]);
    for (int col = 0; col < 3; col++)
      if (row & (1 << (2 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawStrSmall(int x, int y, const char *s, uint8_t bri = 0xFF) {
  const char *start = s;
  while (*s) {
    if (*s == ' ') {
      if (s > start && (*(s - 1) == 'T' || *(s - 1) == 'H')) {
        x += 2; // 2px gap after T and H
      } else {
        x += 4;
      }
    } else {
      drawCharSmall(x, y, *s, bri);
      x += 4;
    }
    s++;
  }
}
int strWSmall(const char *s) {
  int w = 0;
  const char *start = s;
  while (*s) {
    if (*s == ' ') {
      if (s > start && (*(s - 1) == 'T' || *(s - 1) == 'H')) {
        w += 2;
      } else {
        w += 4;
      }
    } else {
      w += 4;
    }
    s++;
  }
  return w > 0 ? w - 1 : 0;
}
int centerXSmall(const char *s) { return (SCREEN_COLS - strWSmall(s)) / 2; }
void drawSymbol3x5(int x, int y, const uint8_t *sym, uint8_t bri = 0xFF) {
  for (int r = 0; r < 5; r++) {
    uint8_t row = pgm_read_byte(sym + r);
    for (int col = 0; col < 3; col++)
      if (row & (1 << (2 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawCharBig(int x, int y, int idx, uint8_t bri = 0xFF) {
  if (idx < 0 || idx > 9)
    return;
  for (int r = 0; r < 9; r++) {
    uint8_t row = pgm_read_byte(&FONT6x9[idx][r]);
    for (int col = 0; col < 6; col++)
      if (row & (1 << (5 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void drawBigHHMM(uint8_t hh, uint8_t mm, uint8_t bri = 0xFF) {
  int y = 1;
  int off = 7;
  drawCharBig(0 + off, y, hh / 10, bri);
  drawCharBig(6 + off, y, hh % 10, bri);
  for (int r = 0; r < 9; r++) {
    if (r == 2 || r == 3 || r == 6 || r == 7) {
      fbSet(13 + off, y + r, bri);
      fbSet(14 + off, y + r, bri);
    }
  }
  drawCharBig(16 + off, y, mm / 10, bri);
  drawCharBig(22 + off, y, mm % 10, bri);
}
void drawIcon7x7(int x, int y, const uint8_t *icon, uint8_t bri = 0xFF) {
  for (int r = 0; r < 7; r++) {
    uint8_t row = pgm_read_byte(icon + r);
    for (int col = 0; col < 7; col++)
      if (row & (1 << (6 - col)))
        fbSet(x + col, y + r, bri);
  }
}
void setupNTP() {
  Serial.println("[NTP] 开始异步同步时钟...");
  configTime(8 * 3600, 0, "ntp.aliyun.com", "time.nist.gov");
  ntpSynced = false;
}
void drawColon3x5(int x, int y) {
  fbSet(x, y + 1, 0xFF);
  fbSet(x, y + 3, 0xFF);
}
static const char *WEEKDAYS[7] = {"SUN", "MON", "TUE", "WED",
                                  "THU", "FRI", "SAT"};
void drawColon5x7Buffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y) {
  if (x >= 0 && x < SCREEN_COLS) {
    if (y + 2 >= 0 && y + 2 < SCREEN_ROWS)
      buf[y + 2][x] = 0xFF;
    if (y + 4 >= 0 && y + 4 < SCREEN_ROWS)
      buf[y + 4][x] = 0xFF;
  }
}
void drawCharSmallBuffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                         char c, uint8_t bri = 0xFF) {
  if (c == ':') {
    if (x + 1 >= 0 && x + 1 < SCREEN_COLS) {
      if (y + 1 >= 0 && y + 1 < SCREEN_ROWS)
        buf[y + 1][x + 1] = bri;
      if (y + 3 >= 0 && y + 3 < SCREEN_ROWS)
        buf[y + 3][x + 1] = bri;
    }
    return;
  } else if (c == '.') {
    if (x + 1 >= 0 && x + 1 < SCREEN_COLS && y + 4 >= 0 &&
        y + 4 < SCREEN_ROWS) {
      buf[y + 4][x + 1] = bri;
    }
    return;
  }
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 5; r++) {
    uint8_t row = pgm_read_byte(&FONT3x5[idx][r]);
    for (int col = 0; col < 3; col++) {
      if (row & (1 << (2 - col))) {
        if (x + col >= 0 && x + col < SCREEN_COLS && y + r >= 0 &&
            y + r < SCREEN_ROWS)
          buf[y + r][x + col] = bri;
      }
    }
  }
}
void drawStrSmallBuffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                        const char *s, uint8_t bri = 0xFF) {
  const char *start = s;
  while (*s) {
    if (*s == ' ') {
      if (s > start && (*(s - 1) == 'T' || *(s - 1) == 'H')) {
        x += 2; // 2px gap after T and H
      } else {
        x += 4;
      }
    } else {
      drawCharSmallBuffer(buf, x, y, *s, bri);
      x += 4;
    }
    s++;
  }
}
void drawChar5x5Buffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                       char c, uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 5; r++) {
    uint8_t row = pgm_read_byte(&FONT5x5[idx][r]);
    for (int col = 0; col < 5; col++) {
      if (row & (1 << (4 - col))) {
        if (x + col >= 0 && x + col < SCREEN_COLS && y + r >= 0 &&
            y + r < SCREEN_ROWS)
          buf[y + r][x + col] = bri;
      }
    }
  }
}
void drawStr5x5Buffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                      const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    if (*s == ':') {
      if (x + 1 >= 0 && x + 1 < SCREEN_COLS) {
        if (y + 1 >= 0 && y + 1 < SCREEN_ROWS)
          buf[y + 1][x + 1] = bri;
        if (y + 3 >= 0 && y + 3 < SCREEN_ROWS)
          buf[y + 3][x + 1] = bri;
      }
      x += 3;
    } else {
      drawChar5x5Buffer(buf, x, y, *s, bri);
      x += 6;
    }
    s++;
  }
}
int strW5x5(const char *s) {
  int w = 0;
  while (*s) {
    if (*s == ':')
      w += 3;
    else
      w += 6;
    s++;
  }
  return w > 0 ? w - 1 : 0;
}
void drawCharBuffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y, char c,
                    uint8_t bri = 0xFF) {
  int idx = -1;
  if (c >= '0' && c <= '9')
    idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    idx = 10 + (c - 'A');
  else if (c >= 'a' && c <= 'z')
    idx = 10 + (c - 'a');
  if (idx < 0)
    return;
  for (int r = 0; r < 7; r++) {
    uint8_t row = pgm_read_byte(&FONT5x7[idx][r]);
    for (int col = 0; col < 5; col++) {
      if (row & (1 << (4 - col))) {
        if (x + col >= 0 && x + col < SCREEN_COLS && y + r >= 0 &&
            y + r < SCREEN_ROWS)
          buf[y + r][x + col] = bri;
      }
    }
  }
}
void drawStrBuffer(uint8_t buf[SCREEN_ROWS][SCREEN_COLS], int x, int y,
                   const char *s, uint8_t bri = 0xFF) {
  while (*s) {
    drawCharBuffer(buf, x, y, *s++, bri);
    x += 6;
  }
}
void copyBufferToFb(uint8_t buf[SCREEN_ROWS][SCREEN_COLS]) {
  memcpy(fb, buf, sizeof(fb));
}
extern uint8_t currentMode;
void processButton();
void fadeTransition(uint8_t old_fb[SCREEN_ROWS][SCREEN_COLS],
                    uint8_t new_fb[SCREEN_ROWS][SCREEN_COLS]) {
  const int duration_ms = 250;
  uint32_t startMs = millis();
  while (true) {
    uint32_t now = millis();
    int prog = (now - startMs) * 100 / duration_ms;
    if (prog > 100)
      prog = 100;
    for (int y = 0; y < SCREEN_ROWS; y++) {
      for (int x = 0; x < SCREEN_COLS; x++) {
        uint32_t o = old_fb[y][x];
        uint32_t n = new_fb[y][x];
        fb[y][x] = (uint8_t)((o * (100 - prog) + n * prog) / 100);
      }
    }
    fbRender();
    processButton();
    if (currentMode != 3)
      return;
    if (prog >= 100)
      break;
    yield();
  }
  copyBufferToFb(new_fb);
  fbRender();
}
enum NtpState { SHOW_TIME, SHOW_DATE };
uint8_t ntp_current_fb[SCREEN_ROWS][SCREEN_COLS];
uint8_t ntp_next_fb[SCREEN_ROWS][SCREEN_COLS];
void runNTPClock() {
  static uint32_t lastSec = 0xFFFFFFFFUL;
  static NtpState dispState = SHOW_TIME;
  static uint32_t stateStartMs = millis();
  static bool firstFrame = true;
  if (firstFrame) {
    memset(ntp_current_fb, 0, sizeof(ntp_current_fb));
    copyBufferToFb(ntp_current_fb);
    fbRender();
    firstFrame = false;
  }
  if (ntpSynced && millis() - ntpLastSyncMs > NTP_RESYNC_MS) {
    setupNTP();
  }
  time_t now = time(nullptr);
  uint32_t sec = (uint32_t)now;
  uint32_t elapsed = millis() - stateStartMs;
  if (dispState == SHOW_TIME && elapsed > 8000) {
    dispState = SHOW_DATE;
    stateStartMs = millis();
  } else if (dispState == SHOW_DATE && elapsed > 4000) {
    dispState = SHOW_TIME;
    stateStartMs = millis();
  }
  static NtpState lastDispState = SHOW_TIME;
  if (sec == lastSec && dispState == lastDispState)
    return;
  lastSec = sec;
  lastDispState = dispState;
  struct tm *ti = localtime(&now);
  memset(ntp_next_fb, 0, sizeof(ntp_next_fb));
  if (WiFi.status() != WL_CONNECTED) {
    drawStrSmallBuffer(ntp_next_fb, 9, 6, "WAIT.");
  } else if (!ntpSynced || now < 1000000000UL) {
    drawStrSmallBuffer(ntp_next_fb, 8, 0, "---");
    if (0 + 1 < SCREEN_ROWS)
      ntp_next_fb[0 + 1][20] = 0xFF;
    if (0 + 3 < SCREEN_ROWS)
      ntp_next_fb[0 + 3][20] = 0xFF;
    drawStrSmallBuffer(ntp_next_fb, 22, 0, "---");
    drawStrSmallBuffer(ntp_next_fb, 9, 6, "NTP...");
  } else {
    if (dispState == SHOW_TIME) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", ti->tm_hour, ti->tm_min,
               ti->tm_sec);
      int sw = strW5x5(buf);
      int cx = (SCREEN_COLS - sw) / 2;
      drawStr5x5Buffer(ntp_next_fb, cx, 3, buf);
    } else {
      char dbuf[16];
      snprintf(dbuf, sizeof(dbuf), "%02d-%02d %s", ti->tm_mon + 1, ti->tm_mday,
               WEEKDAYS[ti->tm_wday]);
      int dw = strWSmall(dbuf);
      drawStrSmallBuffer(ntp_next_fb, (SCREEN_COLS - dw) / 2, 3, dbuf);
    }
  }
  fadeTransition(ntp_current_fb, ntp_next_fb);
  memcpy(ntp_current_fb, ntp_next_fb, sizeof(ntp_current_fb));
}
void enterNTPMode() {
  Serial.println("[模式3] NTP 时钟");
  initAllChips();
  extern uint8_t ntp_current_fb[SCREEN_ROWS][SCREEN_COLS];
  memset(ntp_current_fb, 0, sizeof(ntp_current_fb));
  fbClear();
  fbRender();
  if (WiFi.status() == WL_CONNECTED && !ntpSynced) {
    setupNTP();
  }
}
void scrollStringSmall(int x, int y, const char *s) { drawStrSmall(x, y, s); }
void runAHTMode() {
  static uint32_t lastReadMs = 0;
  static float lastTemp = -999;
  static float lastHum = -999;
  if (millis() - lastReadMs > 2000) {
    lastReadMs = millis();
    if (ahtReady) {
      sensors_event_t h, t;
      aht.getEvent(&h, &t);
      lastTemp = t.temperature;
      lastHum = h.relative_humidity;
    }
  }
  fbClear();
  if (!ahtReady) {
    drawStr3x9(centerX3x9("NA SENSOR"), 1, "NA SENSOR");
  } else {
    char bufT[10];
    char bufTDec[10];
    char bufH[10];

    int tInt = (int)lastTemp;
    int tDec = (int)(lastTemp * 10) % 10;
    if (tDec < 0)
      tDec = -tDec;

    snprintf(bufT, sizeof(bufT), "%d", tInt);
    snprintf(bufTDec, sizeof(bufTDec), "%dC", tDec);
    snprintf(bufH, sizeof(bufH), "%d", (int)lastHum);

    int x = 0;
    drawStr3x9(x, 1, bufT);
    x += strW3x9(bufT) + 1;

    // Draw decimal point
    fbSet(x, 8, 255);
    x += 2;

    drawStr3x9(x, 1, bufTDec);
    x += strW3x9(bufTDec) + 2;

    drawStr3x9(x, 1, bufH);
    x += strW3x9(bufH) + 1;

    drawSymbol3x5(x, 1, SYMBOL_PCT);
  }
  fbRender();
}
void enterAHTMode() {
  Serial.println("[Mode 4] AHT20 Temp & Hum");
  initAllChips();
  fbClear();
  fbRender();
  if (!ahtReady) {
    if (aht.begin()) {
      Serial.println("[AHT20] Init Success");
      ahtReady = true;
    } else {
      Serial.println("[AHT20] No sensor found");
    }
  }
}
float matrixDrops[SCREEN_COLS];
float matrixSpeeds[SCREEN_COLS];
void enterMatrixRain() {
  Serial.println("[Mode 5] Matrix Rain");
  initAllChips();
  for (int x = 0; x < SCREEN_COLS; x++) {
    matrixDrops[x] = random(-20, 0);
    matrixSpeeds[x] = random(40, 120) / 100.0f; // Faster speed
  }
}
void runMatrixRain() {
  static uint32_t lastFrame = 0;
  static uint32_t rainStateMs = millis();
  static int rainLevel = 0; // 0: Heavy, 1: Medium, 2: Light

  if (millis() - lastFrame < 30)
    return;
  lastFrame = millis();

  // Switch rain density every 5 seconds
  if (millis() - rainStateMs > 5000) {
    rainLevel = (rainLevel + 1) % 3;
    rainStateMs = millis();
  }

  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      uint8_t v = fb[y][x];
      // Faster drops means we might need a slightly faster fade to keep trails
      // clean
      int fade = 25;
      if (v > fade)
        fb[y][x] = v - fade;
      else
        fb[y][x] = 0;
    }
  }

  int thresholds[3] = {800, 960,
                       995}; // Heavy, Medium, Light probabilities (out of 1000)
  int currentThreshold = thresholds[rainLevel];

  for (int x = 0; x < SCREEN_COLS; x++) {
    int old_iy = (int)matrixDrops[x];
    matrixDrops[x] += matrixSpeeds[x];
    int iy = (int)matrixDrops[x];

    // Fill the path it moved to prevent broken droplets at high speeds
    for (int py = old_iy; py <= iy; py++) {
      if (py >= 0 && py < SCREEN_ROWS) {
        fbSet(x, py, 255);
      }
    }

    // Respawn logic
    if (matrixDrops[x] > SCREEN_ROWS + 5) {
      matrixDrops[x] = SCREEN_ROWS + 5.1f; // Clamp to prevent overflow
      if (random(1000) > currentThreshold) {
        matrixDrops[x] = random(-10, -1);
        if (rainLevel == 0)
          matrixSpeeds[x] = random(80, 180) / 100.0f; // Heavy: very fast
        else if (rainLevel == 1)
          matrixSpeeds[x] = random(30, 80) / 100.0f; // Medium: normal
        else
          matrixSpeeds[x] = random(15, 50) / 100.0f; // Light: slow
      }
    }
  }

  // Draw Clock Overlay on top of the rain! (Only in Matrix+Clock Mode 2)
  if (ntpSynced && currentMode == 2) {
    static uint32_t stateStartMs = millis();
    static NtpState dispState = SHOW_TIME;
    uint32_t elapsed = millis() - stateStartMs;

    if (dispState == SHOW_TIME && elapsed > 8000) {
      dispState = SHOW_DATE;
      stateStartMs = millis();
    } else if (dispState == SHOW_DATE && elapsed > 4000) {
      dispState = SHOW_TIME;
      stateStartMs = millis();
    }

    time_t now = time(nullptr);
    struct tm *ti = localtime(&now);

    // Clear background for text to make it legible
    // We create a black rectangle where the text will go, slightly larger than
    // the text
    auto clearRect = [](int rx, int ry, int rw, int rh) {
      for (int py = ry; py < ry + rh; py++) {
        for (int px = rx; px < rx + rw; px++) {
          fbSet(px, py, 0); // wipe out rain
        }
      }
    };

    if (dispState == SHOW_TIME) {
      // 6 digits (3px each) + 5 gaps (1px each between digits and colons)
      // width = [digit(3)+space(1)+digit(3)] + space(1)+colon(1)+space(1) +
      // [digit(3)+space(1)+digit(3)] + space(1)+colon(1)+space(1) +
      // [digit(3)+space(1)+digit(3)] HH=7, colon_gap=3, MM=7, colon_gap=3, SS=7
      // -> Total width = 27 px
      int blockW = 27;
      int blockH = 11;
      int sx = (SCREEN_COLS - blockW) / 2;
      clearRect(sx, 0, blockW, blockH); // 0 margin

      char d[2];
      d[1] = 0;
      int x = sx;

      d[0] = '0' + ti->tm_hour / 10;
      drawStr3x9(x, 1, d, 0xFF);
      x += 4;
      d[0] = '0' + ti->tm_hour % 10;
      drawStr3x9(x, 1, d, 0xFF);
      x += 4;

      // Colon 1 (uses 1 pixel col, we jump ahead 1px for visual gap, 1px for
      // colon, 1px for next gap = +3 total block shift) Since x is currently at
      // the gap start, colon goes at x
      fbSet(x, 3, 0xFF);
      fbSet(x, 7, 0xFF);
      x += 2; // Jump over colon (1px) + space (1px)

      d[0] = '0' + ti->tm_min / 10;
      drawStr3x9(x, 1, d, 0xFF);
      x += 4;
      d[0] = '0' + ti->tm_min % 10;
      drawStr3x9(x, 1, d, 0xFF);
      x += 4;

      // Colon 2
      fbSet(x, 3, 0xFF);
      fbSet(x, 7, 0xFF);
      x += 2; // Jump over colon (1px) + space (1px)

      d[0] = '0' + ti->tm_sec / 10;
      drawStr3x9(x, 1, d, 0xFF);
      x += 4;
      d[0] = '0' + ti->tm_sec % 10;
      drawStr3x9(x, 1, d, 0xFF);

    } else {
      // Date and weekday tightly packed. "0214 FRI"
      // width = Month(7) + Day(7) + Space(2) + Weekday(strW)
      char md[6];
      snprintf(md, sizeof(md), "%02d%02d", ti->tm_mon + 1,
               ti->tm_mday); // "0214"

      int w2 = strW3x9(WEEKDAYS[ti->tm_wday]);
      int totalW = 7 + 7 + 2 + w2; // MMDD + space + Weekday
      int sx = (SCREEN_COLS - totalW) / 2;

      clearRect(sx, 0, totalW, 11); // 0 margin black box

      char d[2];
      d[1] = 0;
      int x = sx;
      d[0] = md[0];
      drawStr3x9(x, 1, d, 0xFF);
      x += 4; // M
      d[0] = md[1];
      drawStr3x9(x, 1, d, 0xFF);
      x += 3; // M (no trailing space before Day)
      // wait, standard drawStr adds 1px gap anyway if we just do x+=4. Let's
      // make M and D standard 1px gap, but no big gap between MM and DD.
      x += 1; // So after MM, start DD at x=7

      d[0] = md[2];
      drawStr3x9(x, 1, d, 0xFF);
      x += 4; // D
      d[0] = md[3];
      drawStr3x9(x, 1, d, 0xFF);
      x += 4; // D

      x += 1; // extra +1 makes a 2px total gap before Weekday instead of 1px

      drawStr3x9(x, 1, WEEKDAYS[ti->tm_wday], 0xFF);
    }
  }

  fbRender();
}
void enterWaterRipples() {
  Serial.println("[Mode 6] Water Ripples");
  initAllChips();
}
void runWaterRipples() {
  static float t = 0;
  float cx = SCREEN_COLS / 2.0f - 0.5f;
  float cy = SCREEN_ROWS / 2.0f - 0.5f;
  t += 0.15f;
  if (t > M_PI * 2) {
    t -= M_PI * 2;
  }
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrtf(dx * dx + dy * dy);
      float val = sinf(dist * 0.8f - t);
      uint8_t bri = (uint8_t)((val + 1.0f) * 127.5f);
      fbSet(x, y, bri);
    }
  }
  fbRender();
  smartDelay(20);
}
float barCurrentH[6];
float barTargetH[6];
const int barCenters[6] = {3, 10, 17, 24, 31, 38};

void initAllChips() {
  for (int i = 0; i < NUM_CHIPS; i++) {
    drivers[i]->Init();
    drivers[i]->SetGCC(gccValue);
    drivers[i]->SetLEDMatrixState(LED_STATE::ON);
    drivers[i]->SetLEDMatrixPWM(0);
  }
}
void clearAll() {
  for (int i = 0; i < NUM_CHIPS; i++)
    drivers[i]->SetLEDMatrixPWM(0);
}

// ========== MODE 9: ABM BREATH ==========
void enterBreathMode() {
  Serial.println("[Mode 9] ABM Breath");
  ABM_CONFIG cfg;
  cfg.T1 = T1_840MS;
  cfg.T2 = T2_210MS;
  cfg.T3 = T3_840MS;
  cfg.T4 = T4_420MS;
  cfg.Tbegin = LOOP_BEGIN_T1;
  cfg.Tend = LOOP_END_T3;
  cfg.Times = (uint16_t)ABM_LOOP_FOREVER;
  for (int i = 0; i < NUM_CHIPS; i++) {
    drivers[i]->Init();
    drivers[i]->WritePagedReg(PAGEDREGISTER::CR, (i == 0) ? 0x41 : 0x81);
    drivers[i]->SetGCC(gccValue);
    drivers[i]->SetLEDMatrixState(LED_STATE::ON);
    drivers[i]->SetLEDMatrixPWM(0xFF);
    drivers[i]->ConfigABM(ABM_NUM::NUM_1, &cfg);
    drivers[i]->SetLEDMatrixMode(LED_MODE::ABM1);
  }
  for (int i = NUM_CHIPS - 1; i >= 0; i--) {
    uint8_t cr = (i == 0) ? 0x43 : 0x83;
    drivers[i]->WritePagedReg(PAGEDREGISTER::CR, cr & ~0x02);
    drivers[i]->WritePagedReg(PAGEDREGISTER::CR, cr);
    drivers[i]->WritePagedReg(PAGEDREGISTER::TUR, 0x00);
    Serial.printf("  chip%d %s CR=0x%02X\n", i, i == 0 ? "MASTER" : "SLAVE",
                  cr);
  }
}

uint32_t golGameRef = 0;
void enterGameOfLife() {
  Serial.println("[Mode 5] Game of Life");
  initAllChips();
  // Initialize random cells
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      fb[y][x] = (random(100) > 75) ? 255 : 0;
    }
  }
  golGameRef = millis();
  if (golGameRef == 0)
    golGameRef = 1;
}

void runGameOfLife() {
  static uint32_t lastFrame = 0;
  static uint32_t hashHistory[64] = {0};
  static int historyIdx = 0;
  static uint32_t currentRef = 0;

  if (currentRef != golGameRef) {
    currentRef = golGameRef;
    memset(hashHistory, 0, sizeof(hashHistory));
    historyIdx = 0;
  }

  if (millis() - lastFrame < 150)
    return;
  lastFrame = millis();

  uint8_t nextFb[SCREEN_ROWS][SCREEN_COLS];
  int aliveCount = 0;

  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      int neighbors = 0;
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0)
            continue;
          int nx = x + dx;
          int ny = y + dy;
          if (nx >= 0 && nx < SCREEN_COLS && ny >= 0 && ny < SCREEN_ROWS) {
            if (fb[ny][nx] > 0)
              neighbors++;
          }
        }
      }

      if (fb[y][x] > 0) {
        if (neighbors < 2 || neighbors > 3)
          nextFb[y][x] = 0;
        else {
          nextFb[y][x] = 255;
          aliveCount++;
        }
      } else {
        if (neighbors == 3) {
          nextFb[y][x] = 255;
          aliveCount++;
        } else
          nextFb[y][x] = 0;
      }
    }
  }

  uint32_t currentHash = 2166136261u;
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      fb[y][x] = nextFb[y][x];
      // FNV-1a hash calculation
      currentHash ^= fb[y][x];
      currentHash *= 16777619u;
    }
  }

  bool isLoop = false;
  for (int i = 0; i < 64; i++) {
    if (hashHistory[i] != 0 && hashHistory[i] == currentHash) {
      isLoop = true;
      break;
    }
  }

  hashHistory[historyIdx] = currentHash;
  historyIdx = (historyIdx + 1) % 64;

  // If stuck in any loop up to period-64 OR completely dead
  if (isLoop || aliveCount == 0) {
    if (aliveCount == 0)
      smartDelay(1000);
    else
      smartDelay(3000);
    if (currentMode == 5)
      enterGameOfLife();
  }

  fbRender();
}

// Fire Algorithm specific data
uint8_t firePixels[SCREEN_COLS][SCREEN_ROWS + 2] = {0};

void enterFire() {
  Serial.println("[Mode 6] Fire");
  initAllChips();
  memset(firePixels, 0, sizeof(firePixels));
}

void runFire() {
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame < 45) // Run at a relaxed smooth framerate
    return;
  lastFrame = millis();

  // 1. Generate dense, hot fuel at the very bottom hidden row
  for (int x = 0; x < SCREEN_COLS; x++) {
    // Inject more intense fuel to make the base fuller
    if (random(100) > 20) {
      firePixels[x][SCREEN_ROWS + 1] = random(150, 255);
    } else {
      firePixels[x][SCREEN_ROWS + 1] = random(20, 80); // Occasional cool spots
    }
  }

  // 2. The classic DOOM fire propagation
  // We process from top to bottom (or bottom to top, both work if using a
  // buffer, but in-place DOOM fire works bottom-up usually. Wait, DOOM
  // processes every pixel and looks BELOW it to decide its CURRENT value. Our
  // nested loops below do exactly this by writing to `y` reading from `y+1`)
  for (int x = 0; x < SCREEN_COLS; x++) {
    for (int y = 0; y <= SCREEN_ROWS; y++) {
      int srcIntensity = firePixels[x][y + 1];

      // Random decay determines the "cooling" speed as it rises (higher decay =
      // shorter fire)
      int decay = random(7, 67);
      int newIntensity = srcIntensity - decay;
      if (newIntensity < 0)
        newIntensity = 0;

      // Randomly drift the resulting flame slightly left, right, or straight up
      int drift = random(0, 3) - 1; // -1, 0, 1
      int targetX = x + drift;

      // Wrap around the cylindrical display
      if (targetX < 0)
        targetX = SCREEN_COLS - 1;
      else if (targetX >= SCREEN_COLS)
        targetX = 0;

      // Write the cooled pixel to the current row
      firePixels[targetX][y] = newIntensity;
    }
  }

  // 3. Render out to screen with horizontal softening
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      // 3-tap horizontal smoothing to make the fire look fluid instead of
      // blocky
      int left =
          (x > 0) ? firePixels[x - 1][y] : firePixels[SCREEN_COLS - 1][y];
      int right =
          (x < SCREEN_COLS - 1) ? firePixels[x + 1][y] : firePixels[0][y];
      int center = firePixels[x][y];

      int smoothed = (center * 2 + left + right) / 4;
      fb[y][x] = (uint8_t)smoothed;
    }
  }

  // 4. Draw Information Overlay (Time / Date / Temp)
  static uint32_t stateTimer = 0;
  static int displayState = 0; // 0=Time(8s), 1=Date(4s), 2=AHT(2.5s)
  static uint32_t ahtLastReadMs = 0;
  static float fireLastTemp = 0.0;
  static float fireLastHum = 0.0;

  // Poll AHT every 2 seconds if ready
  if (ahtReady && millis() - ahtLastReadMs > 2000) {
    ahtLastReadMs = millis();
    sensors_event_t h, t;
    aht.getEvent(&h, &t);
    fireLastTemp = t.temperature;
    fireLastHum = h.relative_humidity;
  }

  uint32_t nowMs = millis();
  uint32_t duration =
      (displayState == 0) ? 8000 : (displayState == 1 ? 3000 : 2500);

  if (nowMs - stateTimer > duration) {
    stateTimer = nowMs;
    displayState = (displayState + 1) % 3;
    // Skip states if sensors/NTP are missing
    if (displayState == 2 && !ahtReady)
      displayState = 0;
    if ((displayState == 0 || displayState == 1) && !ntpSynced)
      displayState = (ahtReady ? 2 : 0);
  }

  char buf[32] = {0};
  if (displayState == 0 && ntpSynced) {
    time_t now = time(nullptr);
    struct tm *tInfo = localtime(&now);
    // HH:MM:SS (drawCharSmallBuffer will draw ':' properly now)
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tInfo->tm_hour, tInfo->tm_min,
             tInfo->tm_sec);
  } else if (displayState == 1 && ntpSynced) {
    time_t now = time(nullptr);
    struct tm *tInfo = localtime(&now);
    // JAN 01 SAT
    const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    const char *days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    snprintf(buf, sizeof(buf), "%s %02d %s", months[tInfo->tm_mon],
             tInfo->tm_mday, days[tInfo->tm_wday]);
  } else if (displayState == 2 && ahtReady) {
    // T 25.1 H 45 (1 space gap after T and H as requested)
    snprintf(buf, sizeof(buf), "T %.*f H %.*f", 1, fireLastTemp, 0,
             fireLastHum);
  }

  if (buf[0] != '\0') {
    int sw = strWSmall(buf);
    int cx = (SCREEN_COLS - sw) / 2;
    int cy = 1; // Top aligned

    // Draw the bright 3x5 text directly over the fire without background clear
    drawStrSmallBuffer(fb, cx, cy, buf);
  }

  fbRender();
}

// ========== MODE 9: PONG CLOCK ==========
struct PaddleState {
  float y;   // center y of paddle (float for smooth movement)
  int score; // this padddle's score = time digit displayed
};

static PaddleState pongLeft = {5.0f, 0};
static PaddleState pongRight = {5.0f, 0};
static float pongBallX = 21.0f, pongBallY = 5.0f;
static float pongBallVx = 0.25f, pongBallVy = 0.15f;
static bool pongUpdateScore = false;

void enterPongClock() {
  Serial.println("[Mode 9] Pong Clock");
  initAllChips();
  pongLeft.y = 5.0f;
  pongRight.y = 5.0f;
  pongBallX = 21.0f;
  pongBallY = 5.0f;
  pongBallVx = 0.25f;
  pongBallVy = 0.15f;
  pongUpdateScore = false;
}

void runPongClock() {
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame < 35)
    return;
  lastFrame = millis();

  // Get current time
  int curHour = 0, curMin = 0, curSec = 0;
  if (ntpSynced) {
    time_t now = time(nullptr);
    struct tm *ti = localtime(&now);
    curHour = ti->tm_hour;
    curMin = ti->tm_min;
    curSec = ti->tm_sec;
  }

  // ---- Continuous Milliseconds Timeline ----
  // To avoid 1-second jumps and flickering when NTP `curSec` ticks over,
  // we maintain an internal continuous millisecond timer synced to the minute.

  static uint32_t baseMs = 0;
  static int lastSecObj = -1;
  uint32_t nowMs = millis();

  // When hour/minute/second object updates normally, we lock the baseMs
  // to ensure our internal 0-59999ms counter is anchored.
  if (curSec != lastSecObj) {
    // Re-anchor if we drifted too far, or on first run
    uint32_t expectedMs = curSec * 1000;
    uint32_t currentSimMs = nowMs - baseMs;
    // If the difference is huge (e.g. minute rolled over or just started)
    if (lastSecObj == -1 || curSec < lastSecObj ||
        abs((int)currentSimMs - (int)expectedMs) > 1500) {
      baseMs = nowMs - expectedMs;
    }
    lastSecObj = curSec;
  }

  // totalMs goes from 0 to 59999 over the course of the minute.
  uint32_t totalMs = (nowMs - baseMs) % 60000;
  float totalSec = totalMs / 1000.0f; // 0.000 to 59.999 smooth timeline

  // ---- Cinematic Pong Sequence ----
  // We need the RIGHT side (Minutes) to win the point so the minute score
  // increases. This means the LEFT paddle must miss the ball exactly at 59.0s.
  // We want it to hit the RIGHT paddle exactly at 55.0s.
  // The crossing from 55s to 59s takes exactly 4.0 seconds (very slow), moving
  // Right to Left. For the remaining 0 to 55 seconds, we want crossings to be
  // >2s. 55 seconds divided by 10 crossings = 5.5s per crossing. This means 5
  // full cycles (5 lefts, 5 rights). t=0s -> Right Paddle (moving Left) t=5.5s
  // -> Left Paddle (moving Right) t=11.0s -> Right Paddle
  // ...
  // t=55.0s -> Right Paddle.
  // t=55 to 59s -> Right to Left (takes 4.0s) and Left paddle misses.

  bool movingLeft;
  float progress;

  if (totalSec <= 55.0f) {
    // Phase 1: 0 to 55s (5 full cycles, 5.5s per crossing)
    float phasePos = fmod(totalSec, 11.0f); // 11s per full cycle
    if (phasePos < 5.5f) {
      movingLeft = true; // Right to Left
      progress = phasePos / 5.5f;
    } else {
      movingLeft = false; // Left to Right
      progress = (phasePos - 5.5f) / 5.5f;
    }
  } else {
    // Phase 2: 55s to 59s (1 slow crossing Right to Left taking 4s)
    movingLeft = true;
    progress = (totalSec - 55.0f) / 4.0f;
  }

  if (movingLeft) {
    pongBallX = 39.0f - (37.0f * progress); // 39 down to 2
  } else {
    pongBallX = 2.0f + (37.0f * progress); // 2 up to 39
  }

  // If it's past 59 seconds, let it fly off the screen to the left
  if (totalSec >= 59.0f) {
    pongBallX =
        2.0f - (37.0f * ((totalSec - 59.0f) / 1.0f)); // fly off left over 1s
  }

  // Calculate ball Y (just a simple bounce)
  // Give Y a fixed speed of 3.5 pixels per second for a very lazy vertical
  // float
  float bounceY = fmod(totalSec * 3.5f, (SCREEN_ROWS - 2) * 2);
  if (bounceY > SCREEN_ROWS - 2) {
    pongBallY =
        (SCREEN_ROWS - 2) - (bounceY - (SCREEN_ROWS - 2)); // bouncing down
  } else {
    pongBallY = 0.5f + bounceY; // bouncing up
  }

  // Update scores
  pongLeft.score = curHour;
  pongRight.score = curMin;

  // ---- AI Movement (Perfect Tracking unless missing) ----
  // Left paddle perfectly tracks the ball EXCEPT during the final miss
  float targetL = pongBallY;
  if (totalSec >= 58.0f) {
    targetL = (pongBallY <= 5.0f) ? SCREEN_ROWS - 1.5f
                                  : 1.5f; // Flee at 58s so 59s flyby is clean
  }
  float diffL = targetL - pongLeft.y;
  if (fabsf(diffL) > 0.5f)
    pongLeft.y += (diffL > 0 ? 0.25f : -0.25f);

  // Right paddle perfectly tracks the ball
  float targetR = pongBallY;
  float diffR = targetR - pongRight.y;
  if (fabsf(diffR) > 0.5f)
    pongRight.y += (diffR > 0 ? 0.25f : -0.25f);

  // Clamp paddles
  pongLeft.y = constrain(pongLeft.y, 1.5f, SCREEN_ROWS - 2.5f);
  pongRight.y = constrain(pongRight.y, 1.5f, SCREEN_ROWS - 2.5f);

  // ---- Render ----
  fbClear();

  // Left paddle (3px tall, on row 0..SCREEN_ROWS-2 to leave room for bar)
  for (int d = -1; d <= 1; d++) {
    int py = (int)roundf(pongLeft.y) + d;
    if (py >= 0 && py < SCREEN_ROWS - 1)
      fb[py][1] = 0x40; // Dimmed as requested
  }
  // Right paddle
  for (int d = -1; d <= 1; d++) {
    int py = (int)roundf(pongRight.y) + d;
    if (py >= 0 && py < SCREEN_ROWS - 1)
      fb[py][SCREEN_COLS - 2] = 0x40; // Dimmed as requested
  }

  // Ball (avoid bottom row reserved for progress bar)
  int bx = (int)roundf(pongBallX);
  int by = (int)roundf(pongBallY);
  if (bx >= 0 && bx < SCREEN_COLS && by >= 0 && by < SCREEN_ROWS - 1)
    fb[by][bx] = 0x40; // Dimmed as requested

  // Center divider dots (shifted left by 1 pixel as requested, i.e., x = 20)
  for (int y = 0; y < SCREEN_ROWS - 1; y += 2)
    fb[y][(SCREEN_COLS / 2) - 1] = 0x40;

  // Scores
  char lbuf[8], rbuf[8];
  snprintf(lbuf, sizeof(lbuf), "%02d", pongLeft.score);
  snprintf(rbuf, sizeof(rbuf), "%02d", pongRight.score);
  drawStrSmallBuffer(fb, 8, 2, lbuf);
  drawStrSmallBuffer(fb, 24, 2, rbuf);

  // ---- Seconds Progress Bar (bottom row, row SCREEN_ROWS-1) ----
  // 42 pixels wide, 60 seconds total.
  // Pixel i is fully lit if it falls before the current second position.
  // The "fractional" pixel (the one at the boundary) gets gamma-corrected
  // brightness. Human eye gamma ~2.2: perceived = linear^(1/2.2), so linear =
  // perceived^2.2 For the partial pixel: brightness = powf(fraction, 2.2f)
  {
    // Build a perfectly smooth totalSec from totalMs
    // totalSec is continuous from 0.0 to 59.999, never drops back until
    // rollback
    float pixelPos = totalSec * SCREEN_COLS / 60.0f; // 0.0 .. 42.0

    int fullPixels = (int)pixelPos;
    float fraction = pixelPos - fullPixels;
    // Gamma correction for partial pixel: map 0-1 to 0x00-0x40 via
    // powf(frac, 2.2).
    float gamma = 2.2f;
    uint8_t partialBri = (uint8_t)(powf(fraction, gamma) * 64.0f + 0.5f);

    int barRow = SCREEN_ROWS - 1;
    for (int x = 0; x < SCREEN_COLS; x++) {
      if (x < fullPixels)
        fb[barRow][x] = 0x40; // Dimmed as requested
      else if (x == fullPixels && curSec < 60)
        fb[barRow][x] = partialBri;
      else
        fb[barRow][x] = 0;
    }
  }

  fbRender();
}

// ========== MODE 10: SAND ==========
#define SAND_MAX 462                                // 42*11
static uint16_t sandGrid[SCREEN_ROWS][SCREEN_COLS]; // 0=empty, nonzero=sand
                                                    // (stores brightness)
static int sandCount = 0;

void enterSand() {
  Serial.println("[Mode 8] Sand Simulator");
  initAllChips();
  memset(sandGrid, 0, sizeof(sandGrid));
  sandCount = 0;
}

void runSand() {
  static uint32_t lastFrame = 0;
  static uint32_t lastSpawn = 0;
  if (millis() - lastFrame < 45)
    return;
  lastFrame = millis();

  // Spawn sand: Pick a new random column, with a SLIGHT (15%) chance to reuse
  // the last column
  if (millis() - lastSpawn > 60 && sandCount < SAND_MAX - 2) {
    lastSpawn = millis();
    static int lastSx = -1;
    int sx;
    if (lastSx >= 0 && random(100) < 9) {
      sx = lastSx; // slight tendency to drop in the same spot
    } else {
      sx = random(0, SCREEN_COLS);
    }
    if (sandGrid[0][sx] == 0) {
      sandGrid[0][sx] = random(160, 255);
      sandCount++;
      lastSx = sx;
    } else {
      lastSx = -1;
    }
  }

  // Simulate: process bottom-to-top so older sand settles first
  for (int y = SCREEN_ROWS - 2; y >= 0; y--) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      if (sandGrid[y][x] == 0)
        continue;
      uint16_t val = sandGrid[y][x];
      // Fall straight down
      if (sandGrid[y + 1][x] == 0) {
        sandGrid[y + 1][x] = val;
        sandGrid[y][x] = 0;
      } else {
        // Try to slide to left or right diagonally
        int dir = (random(2) == 0) ? 1 : -1;
        int nx = x + dir;
        if (nx >= 0 && nx < SCREEN_COLS && sandGrid[y + 1][nx] == 0) {
          sandGrid[y + 1][nx] = val;
          sandGrid[y][x] = 0;
        } else {
          nx = x - dir;
          if (nx >= 0 && nx < SCREEN_COLS && sandGrid[y + 1][nx] == 0) {
            sandGrid[y + 1][nx] = val;
            sandGrid[y][x] = 0;
          }
          // Else: stuck, stays put
        }
      }
    }
  }

  // Check if screen is mostly full — clear and restart
  if (sandCount >= SAND_MAX - 10) {
    smartDelay(1500);
    memset(sandGrid, 0, sizeof(sandGrid));
    sandCount = 0;
  }

  // Render
  for (int y = 0; y < SCREEN_ROWS; y++)
    for (int x = 0; x < SCREEN_COLS; x++)
      fb[y][x] = (uint8_t)(sandGrid[y][x] > 255 ? 255 : sandGrid[y][x]);

  fbRender();
}

// ========== MODE 9: WEATHER CLOCK ==========
enum WeatherType { W_SUNNY, W_CLOUDY, W_SNOWY, W_WINDY };

static WeatherType curWeather = W_SUNNY;
static uint32_t lastWeatherChange = 0;

void enterWeatherClock() {
  Serial.println("[Mode 9] Weather Clock");
  initAllChips();
  curWeather = W_SUNNY;
  lastWeatherChange = millis();
}

// Helper to draw a filled circle
void drawFilledCircle(int x0, int y0, int r, uint8_t brightness) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if (x * x + y * y <= r * r) {
        int px = x0 + x;
        int py = y0 + y;
        if (px >= 0 && px < SCREEN_COLS && py >= 0 && py < SCREEN_ROWS) {
          fb[py][px] =
              max((int)fb[py][px], (int)brightness); // blend Additive/Max
        }
      }
    }
  }
}

// Helper to draw a line segment
void drawLineBresenham(int x0, int y0, int x1, int y1, uint8_t brightness) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;
  while (true) {
    if (x0 >= 0 && x0 < SCREEN_COLS && y0 >= 0 && y0 < SCREEN_ROWS) {
      fb[y0][x0] = max((int)fb[y0][x0], (int)brightness);
    }
    if (x0 == x1 && y0 == y1)
      break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

void runSunny() {
  uint32_t t = millis();
  // Draw the Sun core in top-left (-2, -2 to hide center)
  drawFilledCircle(-2, -2, 6, 255);
  drawFilledCircle(-2, -2, 8, 80); // glow

  // Draw 8 rotating rays (Layer 1 is constant, Layer 2 is fixed at 45deg
  // offset) 15 seconds per rotation
  float cycleProgress = (t % 15000) / 15000.0f;
  float angleOffset = cycleProgress * 6.28f;

  for (int i = 0; i < 4; i++) {
    // Layer 1 (Long)
    float a1 = angleOffset + (i * 1.57f);
    drawLineBresenham(-2 + cosf(a1) * 7.0f, -2 + sinf(a1) * 7.0f,
                      -2 + cosf(a1) * 15.0f, -2 + sinf(a1) * 15.0f, 150);

    // Layer 2 (Short, 45deg offset, no fade)
    float a2 = angleOffset + (i * 1.57f) + 0.785f;
    drawLineBresenham(-2 + cosf(a2) * 6.0f, -2 + sinf(a2) * 6.0f,
                      -2 + cosf(a2) * 11.0f, -2 + sinf(a2) * 11.0f, 150);
  }
}

void runCloudy() {
  uint32_t t = millis();
  // Draw 2 detailed complex clouds that look fluffy
  for (int c = 0; c < 2; c++) {
    float shift = (t % 20000) / 20000.0f * 100.0f; // Slower pan
    float basex = (c * 40.0f) + shift;
    int cx = ((int)basex) % 80 - 20; // Pan from -20 to 60

    // Cloud construction: flatter bottom, bumpy top
    drawFilledCircle(cx, 1, 3, 200);     // Left bump
    drawFilledCircle(cx + 4, 0, 4, 255); // Main high bump
    drawFilledCircle(cx + 9, 2, 2, 180); // Right small bump
    drawFilledCircle(cx + 3, 3, 2, 100); // Volume filler

    // Flat shadow bottom
    drawLineBresenham(cx - 2, 4, cx + 10, 4, 60);
    drawLineBresenham(cx - 3, 5, cx + 11, 5, 20);
  }
}

// Snow system
struct SnowParticle {
  float x, y, speed, wobble_phase;
  int life; // for fading when stuck
};
#define MAX_SNOW 30
static SnowParticle snow[MAX_SNOW];
static bool snowInit = false;

void runSnowy() {
  if (!snowInit) {
    for (int i = 0; i < MAX_SNOW; i++) {
      snow[i].x = random(0, SCREEN_COLS);
      snow[i].y = random(-20, 0);
      snow[i].speed = random(12, 40) / 10.0f; // 1.2 to 4.0 px/sec (faster)
      snow[i].wobble_phase = random(0, 628) / 100.0f;
      snow[i].life = 255;
    }
    snowInit = true;
  }

  uint32_t t = millis();
  float dt = 0.05f; // approx 50ms frame time

  for (int i = 0; i < MAX_SNOW; i++) {
    // Check if resting on a bright pixel or ground
    int curX = (int)roundf(snow[i].x);
    int curY = (int)roundf(snow[i].y);
    int nextY = curY + 1;

    bool stuck = false;
    if (nextY >= SCREEN_ROWS) {
      stuck = true; // hit bottom
    } else if (curX >= 0 && curX < SCREEN_COLS && nextY >= 0 &&
               nextY < SCREEN_ROWS) {
      if (fb[nextY][curX] > 100) {
        stuck = true; // hit clock digit
      }
    }

    if (!stuck) {
      snow[i].y += snow[i].speed * dt;
      // Mid-air jiggle: Higher amplitude (1.5f), faster frequency (0.005f)
      snow[i].x += sinf(t * 0.005f + snow[i].wobble_phase) * 1.5f * dt;

      // Wrap horizontally
      if (snow[i].x < 0)
        snow[i].x += SCREEN_COLS;
      if (snow[i].x >= SCREEN_COLS)
        snow[i].x -= SCREEN_COLS;

      snow[i].life = 255; // Keep full life while falling
    } else {
      // It is stuck. No crazy twitching on the ground, just sit and fade!
      snow[i].life -= 15;
      if (snow[i].life <= 0) { // Melted completely
        snow[i].y = -2;
        snow[i].x = random(0, SCREEN_COLS);
        snow[i].life = 255;
      }
    }

    // Render snow (Max brightness 0x40 when fully opaque)
    int py = (int)roundf(snow[i].y);
    int px = (int)roundf(snow[i].x);
    if (px >= 0 && px < SCREEN_COLS && py >= 0 && py < SCREEN_ROWS) {
      // Map life [0, 255] to brightness [0, 0x40]
      uint8_t bri = (uint8_t)((snow[i].life / 255.0f) * 64.0f);
      fb[py][px] = max((int)fb[py][px], (int)bri);
    }
  }
}

// Wind system - Calligraphy "rho" (ρ) shape drawn pixel-by-pixel
struct WindStroke {
  float x, y;
  float scale;
  int progress;       // How many "pixels/segments" of the stroke we've drawn
  int targetProgress; // The full length of the stroke
  int life;           // for fading out at the end
  bool active;
};
#define MAX_WIND 2
static WindStroke winds[MAX_WIND];

// Returns coordinates of the rho stroke at a given segment index 'p' (0 to ~20)
// This creates a 1px thin tail on the left, curving up into an open loop on the
// right.
bool getWindPoint(float bx, float by, float s, int p, int &outX, int &outY) {
  // Tail: horizontal line from left to right (e.g. p = 0 to 10)
  if (p <= 10) {
    outX = roundf(bx + p * s);
    outY = roundf(by);
    return true;
  }
  // Loop up-right
  if (p <= 13) {
    int lp = p - 10;
    outX = roundf(bx + 10 * s + lp * 1.5f * s);
    outY = roundf(by - lp * 1.5f * s);
    return true;
  }
  // Loop up-left (top curve)
  if (p <= 16) {
    int lp = p - 13;
    outX = roundf(bx + 14.5f * s - lp * 1.5f * s);
    outY = roundf(by - 4.5f * s - lp * 0.5f * s);
    return true;
  }
  // Loop down (unclosed, stops before hitting the tail again)
  if (p <= 18) {
    int lp = p - 16;
    outX = roundf(bx + 10.0f * s - lp * 1.0f * s);
    outY = roundf(by - 6.0f * s + lp * 2.0f * s);
    return true;
  }
  return false;
}

void runWindy() {
  for (int i = 0; i < MAX_WIND; i++) {
    if (!winds[i].active) {
      if (random(100) < 3) { // Spawns rarely
        winds[i].active = true;
        winds[i].progress = 0;
        winds[i].targetProgress = 19; // total 19 segments in the rho path
        winds[i].life = 255;
        winds[i].x = random(2, SCREEN_COLS - 20); // must fit the long stroke
        winds[i].y = random(6, SCREEN_ROWS - 2);
        winds[i].scale = random(8, 12) / 10.0f;
      }
    } else {
      // Stroke appears "pixel by pixel" (segment by segment) over time
      if (winds[i].progress < winds[i].targetProgress) {
        winds[i].progress += 1; // Unfurls 1 segment per frame (fast!)
      } else {
        // Fully unfurled, start fading out the whole stroke
        winds[i].life -= 15;
        if (winds[i].life <= 0) {
          winds[i].active = false;
        }
        // Subtle drift while fading
        winds[i].x += 0.2f;
      }

      // Render all segments up to 'progress' connecting them with bresenham
      // lines
      if (winds[i].active && winds[i].progress > 0) {
        int px1, py1, px2, py2;
        getWindPoint(winds[i].x, winds[i].y, winds[i].scale, 0, px1, py1);

        for (int p = 1; p <= winds[i].progress; p++) {
          if (getWindPoint(winds[i].x, winds[i].y, winds[i].scale, p, px2,
                           py2)) {
            // Give it a bright core when fully alive, fading down when life
            // drops
            drawLineBresenham(px1, py1, px2, py2, max(50, winds[i].life));
            px1 = px2;
            py1 = py2;
          }
        }
      }
    }
  }
}

void runWeatherClock() {
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame < 50)
    return; // 20fps for smooth weather
  lastFrame = millis();

  // Auto-switch weather for demonstration purposes
  if (millis() - lastWeatherChange > 15000) {
    curWeather = (WeatherType)(((int)curWeather + 1) % 4);
    snowInit = false; // Reset snow if we wrap around
    for (int i = 0; i < MAX_WIND; i++)
      winds[i].active = false; // Reset wind
    lastWeatherChange = millis();
  }

  fbClear();

  // Base Pass: Backgrounds
  if (curWeather == W_SUNNY) {
    runSunny();
  } else if (curWeather == W_CLOUDY) {
    runCloudy();
  } else if (curWeather == W_SNOWY) {
    // Note: To make snow stick to digits, we must render clock FIRST, then
    // snow!
  }

  // Clock Pass
  time_t nowTime = time(nullptr);
  struct tm *ti = localtime(&nowTime);
  int h = ti->tm_hour;
  int m = ti->tm_min;

  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", h, m);

  // Right-align 3x5 font so there is more room on the left for weather
  // Usually it takes ~5 chars * 4 px = 20 px. Center ~32
  // We use 3x5 instead of 5x5 to make it cleaner with weather.
  drawStrSmall(SCREEN_COLS - strWSmall(timeStr) - 1, 3, timeStr, 255);

  // Foreground Pass: Particles that interact with clock
  if (curWeather == W_SNOWY) {
    runSnowy();
  } else if (curWeather == W_WINDY) {
    runWindy();
  }

  fbRender();
}

#define BTN_DEBOUNCE_MS 35
#define BTN_LONG_MS 800
#define BTN_DOUBLE_MS 400
#define BTN_BRT_MS 30
EasyButton btn(PIN_BUTTON, BTN_DEBOUNCE_MS, false, false);
static bool _brtBounced = false;
static uint32_t _brtMs = 0;
void doShortPress() {
  screenOn = !screenOn;
  applyBrightness();
  Serial.printf("[\u6309\u952e] \u77ed\u6309 \u2192 \u5c4f\u5e55%s\n",
                screenOn ? "\u5f00" : "\u5173");
}
void doDoubleClick() {
  if (!screenOn)
    return;
  currentMode = (currentMode + 1) % 11;
  Serial.printf("[按键] 双击 -> 模式%d\n", currentMode);
  if (currentMode == 0)
    enterNTPMode();
  else if (currentMode == 1)
    enterAHTMode();
  else if (currentMode == 2)
    enterMatrixRain();
  else if (currentMode == 3)
    enterWaterRipples();
  else if (currentMode == 4)
    enterMatrixRain();
  else if (currentMode == 5)
    enterGameOfLife();
  else if (currentMode == 6)
    enterFire();
  else if (currentMode == 7)
    enterPongClock();
  else if (currentMode == 8)
    enterSand();
  else if (currentMode == 9)
    enterWeatherClock();
  else if (currentMode == 10)
    enterBreathMode();
}
void doLongPressStep() {
  if (!screenOn || _brtBounced)
    return;
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
  Serial.printf("[\u6309\u952e] \u957f\u6309 \u2192 GCC=%d(step=%d)\n",
                gccValue, step);
}
static bool _pendingShort = false;
static uint32_t _pendingMs = 0;
void setupButton() {
  btn.onPressed([]() {
    _pendingShort = true;
    _pendingMs = millis();
  });
  btn.onSequence(2, BTN_DOUBLE_MS, []() {
    if (!screenOn)
      return;
    _pendingShort = false;
    doDoubleClick();
  });
  btn.onPressedFor(BTN_LONG_MS, []() {
    _pendingShort = false;
    if (!screenOn)
      return;
    _brtBounced = false;
    static bool firstLongPress = true;
    if (firstLongPress) {
      brtDir = -1; // force down on first use
      firstLongPress = false;
    } else {
      brtDir = -brtDir;
    }
    _brtMs = millis() - BTN_BRT_MS;
    Serial.printf("[按键] 长按?dir=%d\n", brtDir);
  });
  btn.begin();
}
void processButton() {
  btn.read();
  if (_pendingShort && millis() - _pendingMs >= BTN_DOUBLE_MS) {
    _pendingShort = false;
    doShortPress();
  }
  if (screenOn && btn.pressedFor(BTN_LONG_MS)) {
    if (!_brtBounced && millis() - _brtMs >= BTN_BRT_MS) {
      _brtMs = millis();
      doLongPressStep();
    }
  }
  if (btn.wasReleased()) {
    _brtBounced = false;
  }

  // Handle background tasks like WiFi and NTP prints instantly
  static bool wifiWasConnected = false;
  if (!wifiWasConnected && WiFi.status() == WL_CONNECTED) {
    wifiWasConnected = true;
    Serial.printf("[WiFi] Connected, IP=%s\n",
                  WiFi.localIP().toString().c_str());
    setupNTP();
  }
  if (wifiWasConnected && !ntpSynced && time(nullptr) > 1000000000UL) {
    Serial.println("[NTP] 同步成功");
    ntpSynced = true;
    ntpLastSyncMs = millis();
  }
}
void smartDelay(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    processButton();
    yield();
  }
}
void slideIn(const char *s, int y = 2) {
  int tx = centerX(s);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr(x, y, s);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStr(tx, y, s);
  fbRender();
}
void slideOut(const char *s, int y = 2) {
  int sx = centerX(s);
  for (int x = sx; x >= -(strW(s) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr(x, y, s);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  fbRender();
}
void fadeIn(const char *s, int y = 2) {
  int cx = centerX(s);
  for (int b = 0; b <= 255; b += 20) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr(cx, y, s, (uint8_t)min(b, 255));
    fbRender();
    smartDelay(ANIM_MS / 2);
  }
}
void fadeOut(const char *s, int y = 2) {
  int cx = centerX(s);
  for (int b = 255; b >= 0; b -= 20) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr(cx, y, s, (uint8_t)max(b, 0));
    fbRender();
    smartDelay(ANIM_MS / 2);
  }
  fbClear();
  fbRender();
}
void twoLine(const char *top, const char *bot, int holdMs = 400) {
  int tx = centerXSmall(top), bx = centerXSmall(bot);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(x, 0, top);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStrSmall(tx, 0, top);
  fbRender();
  for (int x = -(strWSmall(bot) + 1); x < bx; x += 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(tx, 0, top);
    drawStrSmall(x, 6, bot);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStrSmall(tx, 0, top);
  drawStrSmall(bx, 6, bot);
  fbRender();
  for (int t = 0; t < holdMs; t += 50) {
    processButton();
    if (currentMode != 2)
      return;
    smartDelay(50);
  }
  for (int b = 255; b >= 0; b -= 20) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(tx, 0, top, (uint8_t)max(b, 0));
    drawStrSmall(bx, 6, bot, (uint8_t)max(b, 0));
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  fbRender();
}
void demoBigFont() {
  const uint8_t pairs[3][2] = {{0, 0}, {12, 34}, {23, 59}};
  for (int p = 0; p < 3; p++) {
    uint8_t hh = pairs[p][0], mm = pairs[p][1];
    for (int b = 0; b <= 255; b += 15) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawBigHHMM(hh, mm, (uint8_t)min(b, 255));
      fbRender();
      smartDelay(ANIM_MS);
    }
    for (int t = 0; t < 400; t += 50) {
      processButton();
      if (currentMode != 2)
        return;
      smartDelay(50);
    }
    for (int b = 255; b >= 0; b -= 15) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawBigHHMM(hh, mm, (uint8_t)max(b, 0));
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    fbRender();
  }
}
void demoIcons() {
  const uint8_t *icons[4] = {ICON_SUN, ICON_RAIN, ICON_CLOUD, ICON_SNOW};
  const char *labels[4] = {"SUNNY", "RAIN", "CLOUD", "SNOW"};
  for (int i = 0; i < 4; i++) {
    processButton();
    if (currentMode != 2)
      return;
    int off = 7;
    int lxT = 9 + off;
    for (int b = 0; b <= 255; b += 20) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawIcon7x7(0 + off, 2, icons[i], (uint8_t)min(b, 255));
      fbRender();
      smartDelay(ANIM_MS);
    }
    for (int lx = SCREEN_COLS; lx > lxT; lx -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawIcon7x7(0 + off, 2, icons[i]);
      drawStrSmall(lx, 4, labels[i]);
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    drawIcon7x7(0 + off, 2, icons[i]);
    drawStrSmall(lxT, 4, labels[i]);
    fbRender();
    for (int t = 0; t < 500; t += 50) {
      processButton();
      if (currentMode != 2)
        return;
      smartDelay(50);
    }
    for (int b = 255; b >= 0; b -= 20) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawIcon7x7(0 + off, 2, icons[i], (uint8_t)max(b, 0));
      drawStrSmall(lxT, 4, labels[i], (uint8_t)max(b, 0));
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    fbRender();
  }
}
void runTextDemo() {
  demoBigFont();
  processButton();
  if (currentMode != 2)
    return;
  demoIcons();
  processButton();
  if (currentMode != 2)
    return;
  fadeIn("HELLO");
  smartDelay(250);
  processButton();
  if (currentMode != 2)
    return;
  fadeOut("HELLO");
  slideIn("WORLD");
  smartDelay(250);
  processButton();
  if (currentMode != 2)
    return;
  slideOut("WORLD");
  char grp[5] = {0, 0, 0, 0, 0};
  for (int i = 0; i <= 9; i += 4) {
    grp[0] = '0' + i;
    grp[1] = (i + 1 <= 9) ? '0' + i + 1 : ' ';
    grp[2] = (i + 2 <= 9) ? '0' + i + 2 : ' ';
    grp[3] = (i + 3 <= 9) ? '0' + i + 3 : ' ';
    slideIn(grp);
    smartDelay(150);
    processButton();
    if (currentMode != 2)
      return;
    fadeOut(grp);
    processButton();
    if (currentMode != 2)
      return;
  }
  twoLine("HELLO", "WORLD", 400);
  processButton();
  if (currentMode != 2)
    return;
  twoLine("LED", "DEMO", 400);
  processButton();
  if (currentMode != 2)
    return;
  twoLine("42X11", "ESP12F", 400);
  processButton();
  if (currentMode != 2)
    return;

  // New demo for the 3x5 small font!
  char small_trio[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 26; i += 5) {
    small_trio[0] = 'A' + i;
    small_trio[1] = (i + 1 < 26) ? 'A' + i + 1 : ' ';
    small_trio[2] = (i + 2 < 26) ? 'A' + i + 2 : ' ';
    small_trio[3] = (i + 3 < 26) ? 'A' + i + 3 : ' ';
    small_trio[4] = (i + 4 < 26) ? 'A' + i + 4 : ' ';

    int tx = centerXSmall(small_trio);
    for (int x = SCREEN_COLS; x > tx; x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStrSmall(x, 3, small_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    drawStrSmall(tx, 3, small_trio);
    fbRender();

    smartDelay(200);
    processButton();
    if (currentMode != 2)
      return;

    int sx = tx;
    for (int x = sx; x >= -(strWSmall(small_trio) + 1); x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStrSmall(x, 3, small_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
  }

  char small_numbers[6] = "01234";
  int tx = centerXSmall(small_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(x, 3, small_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStrSmall(tx, 3, small_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strWSmall(small_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(x, 3, small_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  strcpy(small_numbers, "56789");
  tx = centerXSmall(small_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(x, 3, small_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStrSmall(tx, 3, small_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strWSmall(small_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStrSmall(x, 3, small_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  // New demo for the 3x9 font!
  char tall_trio[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 26; i += 5) {
    tall_trio[0] = 'A' + i;
    tall_trio[1] = (i + 1 < 26) ? 'A' + i + 1 : ' ';
    tall_trio[2] = (i + 2 < 26) ? 'A' + i + 2 : ' ';
    tall_trio[3] = (i + 3 < 26) ? 'A' + i + 3 : ' ';
    tall_trio[4] = (i + 4 < 26) ? 'A' + i + 4 : ' ';

    int tx = centerX3x9(tall_trio);
    for (int x = SCREEN_COLS; x > tx; x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStr3x9(x, 1, tall_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    drawStr3x9(tx, 1, tall_trio);
    fbRender();

    smartDelay(200);
    processButton();
    if (currentMode != 2)
      return;

    int sx = tx;
    for (int x = sx; x >= -(strW3x9(tall_trio) + 1); x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStr3x9(x, 1, tall_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
  }

  char tall_numbers[6] = "01234";
  tx = centerX3x9(tall_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr3x9(x, 1, tall_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStr3x9(tx, 1, tall_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strW3x9(tall_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr3x9(x, 1, tall_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  strcpy(tall_numbers, "56789");
  tx = centerX3x9(tall_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr3x9(x, 1, tall_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStr3x9(tx, 1, tall_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strW3x9(tall_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr3x9(x, 1, tall_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  // New demo for the 5x5 font!
  char five_trio[5] = {0, 0, 0, 0, 0};
  for (int i = 0; i < 26; i += 4) {
    five_trio[0] = 'A' + i;
    five_trio[1] = (i + 1 < 26) ? 'A' + i + 1 : ' ';
    five_trio[2] = (i + 2 < 26) ? 'A' + i + 2 : ' ';
    five_trio[3] = (i + 3 < 26) ? 'A' + i + 3 : ' ';

    int tx = centerX5x5(five_trio);
    for (int x = SCREEN_COLS; x > tx; x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStr5x5(x, 3, five_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
    fbClear();
    drawStr5x5(tx, 3, five_trio);
    fbRender();

    smartDelay(200);
    processButton();
    if (currentMode != 2)
      return;

    int sx = tx;
    for (int x = sx; x >= -(strW5x5(five_trio) + 1); x -= 2) {
      processButton();
      if (currentMode != 2)
        return;
      fbClear();
      drawStr5x5(x, 3, five_trio);
      fbRender();
      smartDelay(ANIM_MS);
    }
  }

  char five_numbers[6] = "01234";
  tx = centerX5x5(five_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr5x5(x, 3, five_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStr5x5(tx, 3, five_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strW5x5(five_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr5x5(x, 3, five_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  strcpy(five_numbers, "56789");
  tx = centerX5x5(five_numbers);
  for (int x = SCREEN_COLS; x > tx; x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr5x5(x, 3, five_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }
  fbClear();
  drawStr5x5(tx, 3, five_numbers);
  fbRender();
  smartDelay(200);
  for (int x = tx; x >= -(strW5x5(five_numbers) + 1); x -= 2) {
    processButton();
    if (currentMode != 2)
      return;
    fbClear();
    drawStr5x5(x, 3, five_numbers);
    fbRender();
    smartDelay(ANIM_MS);
  }

  char trio[4] = {0, 0, 0, 0};
  for (int i = 0; i < 26; i += 3) {

    trio[0] = 'A' + i;
    trio[1] = (i + 1 < 26) ? 'A' + i + 1 : ' ';
    trio[2] = (i + 2 < 26) ? 'A' + i + 2 : ' ';
    slideIn(trio);
    smartDelay(200);
    processButton();
    if (currentMode != 2)
      return;
    fadeOut(trio);
    processButton();
    if (currentMode != 2)
      return;
  }
  smartDelay(200);
}
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_SDB, OUTPUT);
  digitalWrite(PIN_SDB, HIGH);
  delay(10);
  setupButton();
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  delay(50);
  Serial.println("IS31FL3733x4 Tester - ESP-12F");
  Serial.printf(
      "I2C Addr: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - Screen %dx%d\n",
      drv0.GetI2CAddress(), drv1.GetI2CAddress(), drv2.GetI2CAddress(),
      drv3.GetI2CAddress(), drv4.GetI2CAddress(), drv5.GetI2CAddress(),
      SCREEN_COLS, SCREEN_ROWS);
  Serial.println("Buttons(IO16): short=power long=dim double=mode");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Connect %s ...\n", WIFI_SSID);
  enterNTPMode();
}
void loop() {
  processButton();
  if (currentMode == 0) {
    processButton();
    if (currentMode != 0)
      return;
    runNTPClock();
  } else if (currentMode == 1) {
    processButton();
    if (currentMode != 1)
      return;
    runAHTMode();
  } else if (currentMode == 2) {
    processButton();
    if (currentMode != 2)
      return;
    runMatrixRain();
  } else if (currentMode == 3) {
    processButton();
    if (currentMode != 3)
      return;
    runWaterRipples();
  } else if (currentMode == 4) {
    processButton();
    if (currentMode != 4)
      return;
    runMatrixRain();
  } else if (currentMode == 5) {
    processButton();
    if (currentMode != 5)
      return;
    runGameOfLife();
  } else if (currentMode == 6) {
    processButton();
    if (currentMode != 6)
      return;
    runFire();
  } else if (currentMode == 7) {
    processButton();
    if (currentMode != 7)
      return;
    runPongClock();
  } else if (currentMode == 8) {
    processButton();
    if (currentMode != 8)
      return;
    runSand();
  } else if (currentMode == 9) {
    processButton();
    if (currentMode != 9)
      return;
    runWeatherClock();
  } else if (currentMode == 10) {
    processButton();
  }
}
