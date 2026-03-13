/**
 * 矩阵雨测试 - 优化版本
 * 调整参数使效果更流畅
 */

#include <Wire.h>

// 引脚定义
#define PIN_SDA 4
#define PIN_SCL 5
#define PIN_SDB 0

// 屏幕参数
#define NUM_CHIPS 6
#define CHIP_COLS 7
#define SCREEN_ROWS 11
#define SCREEN_COLS 42

// I2C读写函数
uint8_t i2c_write_reg(const uint8_t addr, const uint8_t reg, const uint8_t *buf, const uint8_t cnt) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t n = Wire.write(buf, cnt);
  Wire.endTransmission();
  return n;
}

uint8_t i2c_read_reg(const uint8_t addr, const uint8_t reg, uint8_t *buf, const uint8_t cnt) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  uint8_t n = Wire.requestFrom(addr, cnt);
  for (uint8_t i = 0; i < n && i < cnt; i++)
    buf[i] = Wire.read();
  return n;
}

// 简化的IS31FL3733驱动
class SimpleIS31FL3733 {
private:
  uint8_t address;
  
public:
  SimpleIS31FL3733(uint8_t addr1, uint8_t addr2) {
    address = ((0xA0) | ((addr2 & 0x03) << 3) | ((addr1 & 0x03) << 1)) >> 1;
  }
  
  uint8_t getAddress() { return address; }
  
  void init() {
    writeReg(0x00, 0xAE);
    delay(10);
    writeReg(0x00, 0x00);
    writeReg(0x01, 0x00);
    writeReg(0x02, 0x00);
    writeReg(0x00, 0x00);
    writeReg(0x01, 0x01);
  }
  
  void setGCC(uint8_t value) {
    writeReg(0x00, 0x00);
    writeReg(0x02, value);
  }
  
  void setPWM(uint8_t* data) {
    writeReg(0x00, 0x01);
    for (int i = 0; i < 192; i++) {
      writeReg(i, data[i]);
    }
    writeReg(0x00, 0x00);
    writeReg(0x01, 0x01);
  }
  
private:
  void writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }
};

// 6个芯片实例
SimpleIS31FL3733* chips[6];

// 帧缓冲区
uint8_t fb[SCREEN_ROWS][SCREEN_COLS];

// 矩阵雨数据
float matrixDrops[SCREEN_COLS];
float matrixSpeeds[SCREEN_COLS];
int rainLevel = 0;
uint32_t rainStateMs = 0;

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
      for (uint8_t cs = 0; cs < SCREEN_ROWS; cs++) {
        pwmBuf[sw * 16 + cs] = fb[cs][physX];
      }
    }
    chips[chip]->setPWM(pwmBuf);
  }
}

void initAllChips() {
  for (int i = 0; i < NUM_CHIPS; i++) {
    chips[i]->init();
    chips[i]->setGCC(159);
  }
}

void initMatrixRain() {
  for (int x = 0; x < SCREEN_COLS; x++) {
    matrixDrops[x] = random(-15, 5);  // 调整初始位置范围
    matrixSpeeds[x] = random(60, 150) / 100.0f;  // 加快基础速度
  }
  rainLevel = 0;
  rainStateMs = millis();
  fbClear();
}

void runMatrixRain() {
  static uint32_t lastFrame = 0;
  static uint32_t debugMs = 0;
  
  if (millis() - lastFrame < 25) return;  // 加快到40fps
  lastFrame = millis();
  
  // 每5秒切换雨量
  if (millis() - rainStateMs > 5000) {
    rainLevel = (rainLevel + 1) % 3;
    rainStateMs = millis();
    Serial.printf("[矩阵雨] 切换雨量级别: %d\n", rainLevel);
  }
  
  // 淡出效果 - 减小fade值使拖尾更平滑
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      uint8_t v = fb[y][x];
      // 使用更小的fade值，拖尾更长更平滑
      int fade = 15;  // 原来是25
      if (v > fade) fb[y][x] = v - fade;
      else fb[y][x] = 0;
    }
  }
  
  // 雨量参数 - 提高重生概率
  int thresholds[3] = {700, 850, 950};  // 原来是800,960,995
  int currentThreshold = thresholds[rainLevel];
  int activeDrops = 0;
  
  // 更新雨滴
  for (int x = 0; x < SCREEN_COLS; x++) {
    int old_iy = (int)matrixDrops[x];
    matrixDrops[x] += matrixSpeeds[x];
    int iy = (int)matrixDrops[x];
    
    // 填充路径 - 设置头部为最大亮度
    for (int py = old_iy; py <= iy; py++) {
      if (py >= 0 && py < SCREEN_ROWS) {
        fb[py][x] = 255;
      }
    }
    
    // 检查活跃雨滴
    if (iy >= 0 && iy < SCREEN_ROWS) activeDrops++;
    
    // Respawn - 提高重生概率
    if (matrixDrops[x] > SCREEN_ROWS + 3) {  // 原来是+5
      matrixDrops[x] = SCREEN_ROWS + 3.1f;
      if (random(1000) > currentThreshold) {
        matrixDrops[x] = random(-8, -1);  // 减小重生位置范围
        // 加快各档位速度
        if (rainLevel == 0)
          matrixSpeeds[x] = random(100, 200) / 100.0f;  // Heavy: 1.0-2.0
        else if (rainLevel == 1)
          matrixSpeeds[x] = random(50, 100) / 100.0f;   // Medium: 0.5-1.0
        else
          matrixSpeeds[x] = random(30, 60) / 100.0f;    // Light: 0.3-0.6
      }
    }
  }
  
  fbRender();
  
  // 调试输出
  if (millis() - debugMs > 1000) {
    debugMs = millis();
    Serial.printf("[矩阵雨] 活跃: %d, 雨量: %d\n", activeDrops, rainLevel);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n========================================");
  Serial.println("  矩阵雨测试 - 优化版本");
  Serial.println("  优化: 更快速度, 更平滑拖尾, 更高密度");
  Serial.println("========================================\n");

  // 启用SDB
  pinMode(PIN_SDB, OUTPUT);
  digitalWrite(PIN_SDB, HIGH);
  delay(10);

  // 初始化I2C
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  delay(50);

  // 创建芯片实例
  chips[0] = new SimpleIS31FL3733(0, 0);
  chips[1] = new SimpleIS31FL3733(1, 0);
  chips[2] = new SimpleIS31FL3733(2, 0);
  chips[3] = new SimpleIS31FL3733(3, 0);
  chips[4] = new SimpleIS31FL3733(0, 1);
  chips[5] = new SimpleIS31FL3733(1, 1);

  // 初始化芯片
  initAllChips();
  Serial.println("[初始化] IS31FL3733 x6 完成");

  // 初始化矩阵雨
  initMatrixRain();
  Serial.println("[初始化] 矩阵雨已启动");
}

void loop() {
  runMatrixRain();
}
