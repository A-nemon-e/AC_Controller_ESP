/*
 * IS31FL3733 LED Matrix Driver Implementation
 * 修正版：基于neilenns/is31fl3733库和数据手册验证
 */

#include "led_driver.h"
#include "config.h"

// 静态成员初始化
const uint8_t LEDDriver::CHIP_ADDRESSES[6] = {0x50, 0x51, 0x52,
                                              0x53, 0x54, 0x55};
uint8_t LEDDriver::pwmBuffer[6][11][7] = {0};

// IS31FL3733寄存器定义（修正版）
// Common Registers (不分页)
#define REG_PSWL 0xFE    // Page Select Write Lock
#define PSWL_UNLOCK 0xC5 // 解锁魔术值
#define REG_PSR 0xFD     // Page Select Register

// Page Definitions
#define PAGE_LED_CONTROL 0x00 // LED开关控制页面
#define PAGE_PWM 0x01         // PWM亮度页面
#define PAGE_CONFIG 0x03      // 配置页面

// Paged Registers (Page 0x03 - Configuration)
#define REG_CR 0x00    // Configuration Register
#define REG_GCC 0x01   // Global Current Control
#define REG_RESET 0x11 // Reset Register

// Configuration Register Values
#define CR_NORMAL 0x01   // 正常工作模式（SSD=1）
#define CR_SHUTDOWN 0x00 // Shutdown模式

void LEDDriver::init() {
  DEBUG_PRINTLN("[LED驱动] 初始化6个IS31FL3733芯片...");

  for (uint8_t i = 0; i < 6; i++) {
    DEBUG_PRINTF("[LED驱动] 初始化芯片#%d (地址0x%02X)...\n", i + 1,
                 CHIP_ADDRESSES[i]);
    initChip(CHIP_ADDRESSES[i]);
  }

  // 清空缓冲区
  clear();

  DEBUG_PRINTLN("[LED驱动] ✅ 所有芯片初始化完成");
}

void LEDDriver::initChip(uint8_t chipAddr) {
  DEBUG_PRINTF("[LED驱动]   步骤1: 软件复位...\n");
  // 1. 软件复位：读取RESET寄存器触发复位
  selectPage(chipAddr, PAGE_CONFIG);
  Wire.beginTransmission(chipAddr);
  Wire.write(REG_RESET);
  Wire.endTransmission();

  Wire.requestFrom(chipAddr, (uint8_t)1);
  if (Wire.available()) {
    Wire.read(); // 读取触发复位
  }
  delay(10); // 等待复位完成
  DEBUG_PRINTF("[LED驱动]   ✅ 复位完成\n");

  DEBUG_PRINTF("[LED驱动]   步骤2: 清除Software Shutdown...\n");
  // 2. 清除Software Shutdown，设置为正常工作模式
  writePagedRegister(chipAddr, PAGE_CONFIG, REG_CR, CR_NORMAL);
  delay(1);
  DEBUG_PRINTF("[LED驱动]   ✅ 芯片已启用（CR=0x01）\n");

  DEBUG_PRINTF("[LED驱动]   步骤3: 设置全局亮度...\n");
  // 3. 设置全局电流控制(GCC)
  writePagedRegister(chipAddr, PAGE_CONFIG, REG_GCC, LED_BRIGHTNESS_DEFAULT);
  delay(1);
  DEBUG_PRINTF("[LED驱动]   ✅ GCC=%d\n", LED_BRIGHTNESS_DEFAULT);

  DEBUG_PRINTF("[LED驱动]   步骤4: 使能所有LED...\n");
  // 4. 选择LED控制页面，使能所有77个LED
  selectPage(chipAddr, PAGE_LED_CONTROL);

  // LED控制寄存器：每个bit控制一个LED
  // 11×7=77个LED需要0x18=24字节（24×8=192 > 77）
  for (uint8_t reg = 0x00; reg < 0x18; reg++) {
    Wire.beginTransmission(chipAddr);
    Wire.write(reg);
    Wire.write(0xFF); // 全部使能
    Wire.endTransmission();
  }
  delay(1);
  DEBUG_PRINTF("[LED驱动]   ✅ 77个LED已使能\n");

  DEBUG_PRINTF("[LED驱动]   步骤5: 清空PWM寄存器...\n");
  // 5. 选择PWM页面，清空所有PWM（全灭）
  selectPage(chipAddr, PAGE_PWM);

  // PWM寄存器：0x00-0xB3（180个寄存器）
  for (uint8_t reg = 0x00; reg < 0xB4; reg++) {
    Wire.beginTransmission(chipAddr);
    Wire.write(reg);
    Wire.write(0x00); // PWM=0（全灭）
    Wire.endTransmission();
  }
  DEBUG_PRINTF("[LED驱动]   ✅ PWM已清空\n");
}

void LEDDriver::selectPage(uint8_t chipAddr, uint8_t page) {
  // Step 1: 解锁页面选择
  Wire.beginTransmission(chipAddr);
  Wire.write(REG_PSWL);    // PSWL寄存器
  Wire.write(PSWL_UNLOCK); // 写0xC5解锁
  Wire.endTransmission();

  // Step 2: 选择页面
  Wire.beginTransmission(chipAddr);
  Wire.write(REG_PSR); // PSR寄存器
  Wire.write(page);    // 页面号
  Wire.endTransmission();
}

void LEDDriver::writePagedRegister(uint8_t chipAddr, uint8_t page, uint8_t reg,
                                   uint8_t value) {
  // 选择页面
  selectPage(chipAddr, page);

  // 写入寄存器
  Wire.beginTransmission(chipAddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void LEDDriver::setPixel(uint8_t x, uint8_t y, uint8_t brightness) {
  // 参数验证
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
    DEBUG_PRINTF("[LED驱动] ⚠️  坐标超出范围: (%d, %d)\n", x, y);
    return;
  }

  // 计算芯片索引和局部坐标
  uint8_t chipIndex = x / 7; // 每个芯片7列宽
  uint8_t localX = x % 7;    // 芯片内列坐标（SW）
  uint8_t localY = y;        // 芯片内行坐标（CS）

  // 更新缓冲区
  pwmBuffer[chipIndex][localY][localX] = brightness;

  // 计算LED寄存器地址
  // IS31FL3733布局：CS[0-10] × SW[0-6]
  // PWM寄存器地址 = SW * CS_LINES + CS
  uint8_t ledAddr = localX * 16 + localY;

  // 写入PWM寄存器
  uint8_t chipAddr = CHIP_ADDRESSES[chipIndex];
  writePagedRegister(chipAddr, PAGE_PWM, ledAddr, brightness);
}

void LEDDriver::clear() { memset(pwmBuffer, 0, sizeof(pwmBuffer)); }

void LEDDriver::show() {
  // 批量更新所有芯片的PWM值
  for (uint8_t chip = 0; chip < 6; chip++) {
    uint8_t chipAddr = CHIP_ADDRESSES[chip];

    // 选择PWM页面
    selectPage(chipAddr, PAGE_PWM);

    // 优化刷屏：利用地址自增特性，每列(SW)只发送一次起始地址
    // IS31FL3733的PWM寄存器虽然是二维布局，但在I2C地址空间中是连续的
    // CS(Row)是低位地址，SW(Col)是高位地址 (Addr = SW * 16 + CS)
    // 只要连续写入数据，内部地址指针会自动增加

    for (uint8_t col = 0; col < 7; col++) {
      // 计算该列的起始寄存器地址 (CS1/Row0 的位置)
      uint8_t startReg = col * 16;

      Wire.beginTransmission(chipAddr);
      Wire.write(startReg); // 写入起始地址

      // 连续写入该列的所有行(CS0-CS10)的数据
      // I2C协议中，后续的字节会被依次写入 startReg, startReg+1, ...
      for (uint8_t row = 0; row < 11; row++) {
        Wire.write(pwmBuffer[chip][row][col]);
      }

      Wire.endTransmission();
    }
  }
}

void LEDDriver::setBrightness(uint8_t value) {
  DEBUG_PRINTF("[LED驱动] 设置全局亮度: %d\n", value);

  for (uint8_t i = 0; i < 6; i++) {
    writePagedRegister(CHIP_ADDRESSES[i], PAGE_CONFIG, REG_GCC, value);
  }
}

void LEDDriver::test() {
  DEBUG_PRINTLN("[LED驱动] 测试：点亮左上角LED (0,0)");

  // 点亮第一个像素
  setPixel(0, 0, 255);

  DEBUG_PRINTLN("[LED驱动] ✅ 测试完成，应看到左上角LED点亮");
}
