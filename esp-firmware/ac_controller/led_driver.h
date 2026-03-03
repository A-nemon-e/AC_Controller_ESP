/*
 * IS31FL3733 LED Matrix Driver
 *
 * 驱动6个IS31FL3733芯片，构成42×11 LED矩阵
 * I2C地址：0x50-0x55
 */

#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

class LEDDriver {
public:
  // 初始化所有6个LED驱动芯片
  static void init();

  // 设置单个像素亮度
  // x: 0-41 (列坐标)
  // y: 0-10 (行坐标)
  // brightness: 0-255 (PWM亮度)
  static void setPixel(uint8_t x, uint8_t y, uint8_t brightness);

  // 清空所有像素（设为0）
  static void clear();

  // 刷新显示（将缓冲区写入芯片）
  static void show();

  // 设置全局亮度
  // value: 0-255 (GCC寄存器)
  static void setBrightness(uint8_t value);

  // 测试函数：点亮左上角LED
  static void test();

private:
  // 6个芯片的I2C地址
  static const uint8_t CHIP_ADDRESSES[6];

  // PWM缓冲区：6芯片 × 11行 × 7列
  static uint8_t pwmBuffer[6][11][7];

  // 初始化单个芯片
  static void initChip(uint8_t chipAddr);

  // 选择页面（先解锁PSWL，再写PSR）
  static void selectPage(uint8_t chipAddr, uint8_t page);

  // 写入分页寄存器
  static void writePagedRegister(uint8_t chipAddr, uint8_t page, uint8_t reg,
                                 uint8_t value);

  // 写入寄存器
  // addr: I2C地址
  // page: 页面号（0x00/0x01/0x03/0xFD）
  // reg: 寄存器地址
  // value: 写入值
  static void writeRegister(uint8_t addr, uint8_t page, uint8_t reg,
                            uint8_t value);

  // 计算LED在芯片内的寄存器地址
  // col: 0-6 (SW列)
  // row: 0-10 (CS行)
  // 返回：PWM寄存器地址
  static uint8_t getLEDAddress(uint8_t col, uint8_t row);
};

#endif // LED_DRIVER_H
