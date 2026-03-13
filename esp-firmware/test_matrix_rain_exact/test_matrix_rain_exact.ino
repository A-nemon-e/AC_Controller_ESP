/**
 * 矩阵雨测试 - 完全照搬 test_3733_scanner
 */

#include <Wire.h>

#define PIN_SDA 4
#define PIN_SCL 5
#define PIN_SDB 0
#define NUM_CHIPS 6
#define CHIP_COLS 7
#define SCREEN_ROWS 11
#define SCREEN_COLS 42

// I2C
uint8_t i2c_write_reg(const uint8_t addr, const uint8_t reg, const uint8_t *buf, const uint8_t cnt) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t n = Wire.write(buf, cnt);
  Wire.endTransmission();
  return n;
}

// 驱动
class SimpleDriver {
  uint8_t addr;
public:
  SimpleDriver(uint8_t a1, uint8_t a2) {
    addr = ((0xA0) | ((a2 & 0x03) << 3) | ((a1 & 0x03) << 1)) >> 1;
  }
  void init() {
    Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0xAE); Wire.endTransmission(); delay(10);
    Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(addr); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(addr); Wire.write(0x02); Wire.write(0xFF); Wire.endTransmission();
    Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(addr); Wire.write(0x01); Wire.write(0x01); Wire.endTransmission();
  }
  void setPWM(uint8_t* data) {
    Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0x01); Wire.endTransmission();
    for (int i = 0; i < 192; i++) {
      Wire.beginTransmission(addr); Wire.write(i); Wire.write(data[i]); Wire.endTransmission();
    }
    Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(addr); Wire.write(0x01); Wire.write(0x01); Wire.endTransmission();
  }
};

SimpleDriver* chips[6];
uint8_t fb[SCREEN_ROWS][SCREEN_COLS];

// 矩阵雨 - 完全照搬 test_3733_scanner
float matrixDrops[SCREEN_COLS];
float matrixSpeeds[SCREEN_COLS];

void fbClear() { memset(fb, 0, sizeof(fb)); }
void fbSet(int x, int y, uint8_t v) {
  if (x >= 0 && x < SCREEN_COLS && y >= 0 && y < SCREEN_ROWS) fb[y][x] = v;
}

void fbRender() {
  uint8_t pwm[192];
  for (uint8_t c = 0; c < 6; c++) {
    memset(pwm, 0, 192);
    for (int8_t sw = 6; sw >= 0; sw--) {
      uint8_t px = c * 7 + (6 - sw);
      for (uint8_t cs = 0; cs < 11; cs++) pwm[sw * 16 + cs] = fb[cs][px];
    }
    chips[c]->setPWM(pwm);
  }
}

void initChips() {
  for (int i = 0; i < 6; i++) chips[i]->init();
}

// 完全照搬 test_3733_scanner
void enterMatrixRain() {
  for (int x = 0; x < SCREEN_COLS; x++) {
    matrixDrops[x] = random(-20, 0);
    matrixSpeeds[x] = random(40, 120) / 100.0f;
  }
  fbClear();
}

void runMatrixRain() {
  static uint32_t lastFrame = 0;
  static uint32_t rainStateMs = millis();
  static int rainLevel = 0;
  
  if (millis() - lastFrame < 30) return;
  lastFrame = millis();
  
  if (millis() - rainStateMs > 5000) {
    rainLevel = (rainLevel + 1) % 3;
    rainStateMs = millis();
  }
  
  // 淡出 - fade = 25
  for (int y = 0; y < SCREEN_ROWS; y++) {
    for (int x = 0; x < SCREEN_COLS; x++) {
      uint8_t v = fb[y][x];
      if (v > 25) fb[y][x] = v - 25;
      else fb[y][x] = 0;
    }
  }
  
  // 照搬 threshold
  int thresholds[3] = {800, 960, 995};
  int currentThreshold = thresholds[rainLevel];
  
  for (int x = 0; x < SCREEN_COLS; x++) {
    int old_iy = (int)matrixDrops[x];
    matrixDrops[x] += matrixSpeeds[x];
    int iy = (int)matrixDrops[x];
    
    // 填充路径
    for (int py = old_iy; py <= iy; py++) {
      if (py >= 0 && py < SCREEN_ROWS) fbSet(x, py, 255);
    }
    
    // Respawn - 照搬
    if (matrixDrops[x] > SCREEN_ROWS + 5) {
      matrixDrops[x] = SCREEN_ROWS + 5.1f;
      if (random(1000) > currentThreshold) {
        matrixDrops[x] = random(-10, -1);
        if (rainLevel == 0) matrixSpeeds[x] = random(80, 180) / 100.0f;
        else if (rainLevel == 1) matrixSpeeds[x] = random(30, 80) / 100.0f;
        else matrixSpeeds[x] = random(15, 50) / 100.0f;
      }
    }
  }
  
  fbRender();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  pinMode(PIN_SDB, OUTPUT);
  digitalWrite(PIN_SDB, HIGH);
  delay(10);
  
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  delay(50);
  
  chips[0] = new SimpleDriver(0, 0);
  chips[1] = new SimpleDriver(1, 0);
  chips[2] = new SimpleDriver(2, 0);
  chips[3] = new SimpleDriver(3, 0);
  chips[4] = new SimpleDriver(0, 1);
  chips[5] = new SimpleDriver(1, 1);
  
  initChips();
  enterMatrixRain();
  
  Serial.println("矩阵雨 - 完全照搬 test_3733_scanner");
}

void loop() {
  runMatrixRain();
}
