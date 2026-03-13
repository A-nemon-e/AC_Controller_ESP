/**
 * LEDMatrix - LED矩阵Framebuffer封装
 * 
 * 实现文件
 */

#include "LEDMatrix.h"

// 静态成员初始化
uint8_t LEDMatrix::buffer[SCREEN_ROWS][SCREEN_COLS] = {{0}};
IS31FL3733::IS31FL3733Driver* LEDMatrix::drivers[NUM_CHIPS] = {nullptr};
bool LEDMatrix::initialized = false;
bool LEDMatrix::screenOn = true;
uint8_t LEDMatrix::globalBrightness = 128;
LEDMatrixError LEDMatrix::lastError = LEDMatrixError::NONE;
uint32_t LEDMatrix::frameCounter = 0;
uint32_t LEDMatrix::errorCounter = 0;

// I2C地址定义 - 根据ADDR1/ADDR2引脚连接
const uint8_t CHIP_ADDRESSES[NUM_CHIPS] = {
    0x50,  // chip5: ADDR1=GND, ADDR2=GND
    0x51,  // chip4: ADDR1=SCL, ADDR2=GND
    0x52,  // chip3: ADDR1=SDA, ADDR2=GND
    0x53,  // chip2: ADDR1=VCC, ADDR2=GND
    0x54,  // chip1: ADDR1=GND, ADDR2=SCL
    0x55   // chip0: ADDR1=SCL, ADDR2=SCL
};

bool LEDMatrix::begin() {
    if (initialized) {
        return true;
    }

    // 清空错误状态
    clearError();

    // 初始化I2C总线（ESP8266需要指定SDA和SCL引脚）
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);  // 400kHz快速模式

    // 扫描I2C设备（调试用）
    #ifdef DEBUG_LED_MATRIX
    scanI2C();
    #endif

    // 启用SDB引脚（解除复位）
    pinMode(PIN_SDB, OUTPUT);
    digitalWrite(PIN_SDB, HIGH);
    delay(10);  // 等待芯片稳定

    // 创建驱动实例（注意顺序：chip5到chip0）
    for (int i = NUM_CHIPS - 1; i >= 0; i--) {
        uint8_t addr1, addr2;
        
        // 根据地址计算ADDR引脚配置
        switch (CHIP_ADDRESSES[i] & 0x0F) {
            case 0x00: addr1 = IS31FL3733::ADDR::GND; addr2 = IS31FL3733::ADDR::GND; break;
            case 0x01: addr1 = IS31FL3733::ADDR::SCL; addr2 = IS31FL3733::ADDR::GND; break;
            case 0x02: addr1 = IS31FL3733::ADDR::SDA; addr2 = IS31FL3733::ADDR::GND; break;
            case 0x03: addr1 = IS31FL3733::ADDR::VCC; addr2 = IS31FL3733::ADDR::GND; break;
            case 0x04: addr1 = IS31FL3733::ADDR::GND; addr2 = IS31FL3733::ADDR::SCL; break;
            case 0x05: addr1 = IS31FL3733::ADDR::SCL; addr2 = IS31FL3733::ADDR::SCL; break;
            default: addr1 = IS31FL3733::ADDR::GND; addr2 = IS31FL3733::ADDR::GND; break;
        }

        drivers[i] = new IS31FL3733::IS31FL3733Driver(
            static_cast<IS31FL3733::ADDR>(addr1),
            static_cast<IS31FL3733::ADDR>(addr2),
            i2cReadReg,
            i2cWriteReg
        );

        // 初始化芯片
        drivers[i]->Init();

        // 设置全局亮度
        drivers[i]->SetGCC(globalBrightness);

        // 启用上拉/下拉电阻
        drivers[i]->SetSWPUR(IS31FL3733::RESISTOR::RESISTOR_1K);
        drivers[i]->SetCSPDR(IS31FL3733::RESISTOR::RESISTOR_1K);

        // 关键：打开LED矩阵（参考test_3733_scanner的initAllChips）
        drivers[i]->SetLEDMatrixState(IS31FL3733::LED_STATE::ON);
        drivers[i]->SetLEDMatrixPWM(0);

        // 关闭软件关断
        drivers[i]->WritePagedReg(
            IS31FL3733::PAGEDREGISTER::CR,
            IS31FL3733::CR_OPTIONS::CR_SSD
        );

        // 喂狗，防止初始化超时
        ESP.wdtFeed();
    }

    // 清空缓冲区
    clear();

    initialized = true;
    
    #ifdef DEBUG_LED_MATRIX
    Serial.println("[LEDMatrix] Initialized successfully");
    #endif

    return true;
}

void LEDMatrix::end() {
    if (!initialized) return;

    // 关闭所有芯片
    for (int i = 0; i < NUM_CHIPS; i++) {
        if (drivers[i]) {
            // 软件关断
            drivers[i]->WritePagedReg(
                IS31FL3733::PAGEDREGISTER::CR,
                0
            );
            delete drivers[i];
            drivers[i] = nullptr;
        }
    }

    // SDB引脚拉低（复位芯片）
    digitalWrite(PIN_SDB, LOW);

    initialized = false;
}

void ICACHE_FLASH_ATTR LEDMatrix::clear() {
    memset(buffer, 0, sizeof(buffer));
}

void ICACHE_FLASH_ATTR LEDMatrix::fill(uint8_t brightness) {
    memset(buffer, brightness, sizeof(buffer));
}

void ICACHE_FLASH_ATTR LEDMatrix::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness) {
    // 边界检查
    if (x >= SCREEN_COLS || y >= SCREEN_ROWS) return;
    
    // 裁剪到屏幕范围
    uint8_t x2 = min((uint8_t)(x + w), (uint8_t)SCREEN_COLS);
    uint8_t y2 = min((uint8_t)(y + h), (uint8_t)SCREEN_ROWS);
    
    for (uint8_t row = y; row < y2; row++) {
        for (uint8_t col = x; col < x2; col++) {
            buffer[row][col] = brightness;
        }
    }
}

bool LEDMatrix::setPixel(uint8_t x, uint8_t y, uint8_t brightness) {
    if (!isValidPosition(x, y)) {
        setError(LEDMatrixError::OUT_OF_BOUNDS);
        return false;
    }
    
    buffer[y][x] = brightness;
    return true;
}

uint8_t LEDMatrix::getPixel(uint8_t x, uint8_t y) {
    if (!isValidPosition(x, y)) {
        return 0;
    }
    return buffer[y][x];
}

bool ICACHE_FLASH_ATTR LEDMatrix::setPixelClipped(int16_t x, int16_t y, uint8_t brightness) {
    // 裁剪检查
    if (x < 0 || x >= SCREEN_COLS || y < 0 || y >= SCREEN_ROWS) {
        return false;
    }
    
    buffer[y][x] = brightness;
    return true;
}

bool LEDMatrix::refresh() {
    if (!initialized) {
        setError(LEDMatrixError::NOT_INITIALIZED);
        return false;
    }

    // 喂狗，防止长时间刷新触发复位
    ESP.wdtFeed();

    // 为每个芯片准备PWM数据并发送
    for (int chip = 0; chip < NUM_CHIPS; chip++) {
        uint8_t pwmBuffer[192];  // 192字节 PWM数据 (16 CS x 12 SW)
        
        // 将缓冲区数据转换为芯片PWM格式
        bufferToPWM(chip, pwmBuffer);
        
        // 发送PWM数据
        drivers[chip]->SetPWM(pwmBuffer);
        
        // 每芯片喂狗一次
        ESP.wdtFeed();
    }

    frameCounter++;
    return true;
}

void ICACHE_FLASH_ATTR LEDMatrix::copyFrom(const uint8_t source[SCREEN_ROWS][SCREEN_COLS]) {
    memcpy(buffer, source, sizeof(buffer));
}

void ICACHE_FLASH_ATTR LEDMatrix::setGlobalBrightness(uint8_t gcc) {
    globalBrightness = gcc;
    
    if (!initialized) return;
    
    for (int i = 0; i < NUM_CHIPS; i++) {
        if (drivers[i]) {
            drivers[i]->SetGCC(screenOn ? gcc : 0);
        }
    }
}

void LEDMatrix::setScreenOn(bool on) {
    screenOn = on;
    setGlobalBrightness(globalBrightness);
}

const char* ICACHE_FLASH_ATTR LEDMatrix::getErrorString(LEDMatrixError error) {
    switch (error) {
        case LEDMatrixError::NONE: return "No error";
        case LEDMatrixError::OUT_OF_BOUNDS: return "Position out of bounds";
        case LEDMatrixError::I2C_ERROR: return "I2C communication error";
        case LEDMatrixError::NOT_INITIALIZED: return "Matrix not initialized";
        case LEDMatrixError::BUFFER_OVERFLOW: return "Buffer overflow";
        default: return "Unknown error";
    }
}

void ICACHE_FLASH_ATTR LEDMatrix::testPattern() {
    if (!initialized) return;

    // 逐个点亮所有LED
    for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            clear();
            setPixel(x, y, 128);
            refresh();
            delay(50);
        }
    }
    
    // 全亮测试
    fill(128);
    refresh();
    delay(500);
    
    // 恢复清空
    clear();
    refresh();
}

void ICACHE_FLASH_ATTR LEDMatrix::scanI2C() {
    Serial.println("[LEDMatrix] I2C Scan:");
    
    uint8_t error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("  Found device at 0x%02X\n", address);
            nDevices++;
        }
    }

    if (nDevices == 0) {
        Serial.println("  No I2C devices found!");
    }
}

void ICACHE_FLASH_ATTR LEDMatrix::getStats(uint32_t& frameCount, uint32_t& errorCount) {
    frameCount = frameCounter;
    errorCount = errorCounter;
}

// 私有方法实现

void LEDMatrix::setError(LEDMatrixError error) {
    lastError = error;
    if (error != LEDMatrixError::NONE) {
        errorCounter++;
    }
}

bool LEDMatrix::isValidPosition(uint8_t x, uint8_t y) {
    return x < SCREEN_COLS && y < SCREEN_ROWS;
}

void ICACHE_FLASH_ATTR LEDMatrix::bufferToPWM(uint8_t chipIndex, uint8_t* pwmBuffer) {
    // 完全参照 test_3733_scanner 的 fbRender() 实现
    // chip0 对应右边 (列35-41), chip5 对应左边 (列0-6)
    // sw (scan line) 对应列，但顺序反转
    // cs (current source) 对应行
    
    memset(pwmBuffer, 0, 192);
    
    for (int8_t sw = CHIP_COLS - 1; sw >= 0; sw--) {
        // 计算物理列位置 (参照 test_3733_scanner: physX = chip * CHIP_COLS + (CHIP_COLS - 1 - sw))
        uint8_t physX = chipIndex * CHIP_COLS + (CHIP_COLS - 1 - sw);
        
        for (uint8_t cs = 0; cs < SCREEN_ROWS; cs++) {
            // 应用全局亮度
            uint16_t brightness = buffer[cs][physX];
            brightness = (brightness * globalBrightness) / 255;
            
            // PWM缓冲区布局: sw * 16 + cs (参照 test_3733_scanner)
            pwmBuffer[sw * 16 + cs] = (uint8_t)brightness;
        }
    }
}

// I2C回调函数
// 注意：IS31FL3733库在Arduino模式下已经将地址转换为7位（右移1位）
// 所以这里直接使用addr，不要再右移！
uint8_t LEDMatrix::i2cWriteReg(uint8_t addr, uint8_t reg, const uint8_t* buf, uint8_t cnt) {
    Wire.beginTransmission(addr);  // addr已经是7位地址
    Wire.write(reg);
    for (uint8_t i = 0; i < cnt; i++) {
        Wire.write(buf[i]);
    }
    return Wire.endTransmission();
}

uint8_t LEDMatrix::i2cReadReg(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t cnt) {
    Wire.beginTransmission(addr);  // addr已经是7位地址
    Wire.write(reg);
    uint8_t result = Wire.endTransmission(false);
    
    if (result != 0) return result;
    
    Wire.requestFrom(addr, cnt);
    uint8_t i = 0;
    while (Wire.available() && i < cnt) {
        buf[i++] = Wire.read();
    }
    
    return 0;
}
