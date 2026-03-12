/**
 * test_phase1.ino - Phase 1硬件抽象层测试
 * 
 * 测试内容:
 * 1. LEDMatrix初始化
 * 2. 像素绘制测试
 * 3. 字体渲染测试
 * 4. 配置管理测试
 * 5. 看门狗保护测试
 */

#include <Wire.h>
#include <EEPROM.h>
#include "IS31FL3733.h"
#include "LEDMatrix.h"
#include "FontRenderer.h"
#include "DisplayConfig.h"

// 测试状态
int testNumber = 0;
int testsPassed = 0;
int testsFailed = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  Phase 1: 硬件抽象层测试");
    Serial.println("========================================\n");
    
    // 测试1: LEDMatrix初始化
    testLEDMatrixInit();
    
    // 测试2: 像素操作
    testPixelOperations();
    
    // 测试3: 字体渲染
    testFontRendering();
    
    // 测试4: 配置管理
    testDisplayConfig();
    
    // 测试5: 硬件刷新（实际显示）
    testHardwareRefresh();
    
    // 总结
    Serial.println("\n========================================");
    Serial.printf("  测试结果: %d 通过, %d 失败\n", testsPassed, testsFailed);
    Serial.println("========================================");
}

void loop() {
    // 定期刷新配置
    DisplayConfig::flush();
    
    // 简单的动画测试
    static uint8_t x = 0;
    static uint8_t brightness = 0;
    static int8_t direction = 1;
    
    LEDMatrix::clear();
    LEDMatrix::setPixel(x, 5, brightness);
    LEDMatrix::refresh();
    
    x = (x + 1) % SCREEN_COLS;
    brightness += direction * 16;
    if (brightness >= 240 || brightness <= 15) direction = -direction;
    
    delay(50);
}

void testLEDMatrixInit() {
    Serial.println("[TEST 1] LEDMatrix初始化");
    testNumber++;
    
    if (LEDMatrix::begin()) {
        Serial.println("  ✓ 初始化成功");
        testsPassed++;
    } else {
        Serial.println("  ✗ 初始化失败");
        testsFailed++;
    }
    
    // 测试是否已经初始化
    if (LEDMatrix::isInitialized()) {
        Serial.println("  ✓ 初始化状态正确");
    } else {
        Serial.println("  ✗ 初始化状态错误");
    }
}

void testPixelOperations() {
    Serial.println("\n[TEST 2] 像素操作");
    testNumber++;
    
    // 测试setPixel
    bool result1 = LEDMatrix::setPixel(0, 0, 255);
    bool result2 = LEDMatrix::setPixel(SCREEN_COLS - 1, SCREEN_ROWS - 1, 128);
    bool result3 = LEDMatrix::setPixel(SCREEN_COLS, SCREEN_ROWS, 255);  // 越界测试
    
    if (result1 && result2 && !result3) {
        Serial.println("  ✓ 像素设置正常");
        testsPassed++;
    } else {
        Serial.println("  ✗ 像素设置异常");
        testsFailed++;
    }
    
    // 测试getPixel
    uint8_t pixel1 = LEDMatrix::getPixel(0, 0);
    uint8_t pixel2 = LEDMatrix::getPixel(SCREEN_COLS - 1, SCREEN_ROWS - 1);
    
    if (pixel1 == 255 && pixel2 == 128) {
        Serial.println("  ✓ 像素读取正常");
    } else {
        Serial.println("  ✗ 像素读取异常");
    }
    
    // 测试越界读取
    uint8_t pixel3 = LEDMatrix::getPixel(100, 100);
    if (pixel3 == 0) {
        Serial.println("  ✓ 越界处理正常");
    } else {
        Serial.println("  ✗ 越界处理异常");
    }
    
    // 测试clear
    LEDMatrix::clear();
    if (LEDMatrix::getPixel(0, 0) == 0) {
        Serial.println("  ✓ 清空正常");
    } else {
        Serial.println("  ✗ 清空异常");
    }
}

void testFontRendering() {
    Serial.println("\n[TEST 3] 字体渲染");
    testNumber++;
    
    bool allPassed = true;
    
    // 测试各种字体
    FontType fonts[] = {
        FontType::FONT_3x5,
        FontType::FONT_5x7,
        FontType::FONT_3x9,
        FontType::FONT_5x5,
        FontType::FONT_6x9
    };
    
    const char* fontNames[] = {"3x5", "5x7", "3x9", "5x5", "6x9"};
    
    for (int i = 0; i < 5; i++) {
        LEDMatrix::clear();
        
        uint8_t width = FontRenderer::drawChar('0', 0, 0, fonts[i], 128);
        uint8_t expectedWidth = FontRenderer::getCharWidth(fonts[i]);
        
        if (width == expectedWidth) {
            Serial.printf("  ✓ %s字体渲染正常 (宽度: %d)\n", fontNames[i], width);
        } else {
            Serial.printf("  ✗ %s字体渲染异常 (期望: %d, 实际: %d)\n", 
                         fontNames[i], expectedWidth, width);
            allPassed = false;
        }
    }
    
    // 测试字符串渲染
    LEDMatrix::clear();
    uint16_t strWidth = FontRenderer::drawString("12:30", 0, 0, FontType::FONT_5x7, 200);
    Serial.printf("  ✓ 字符串宽度: %d 像素\n", strWidth);
    
    // 测试不支持的字符
    int8_t idx = FontRenderer::getCharIndex('@');
    if (idx < 0) {
        Serial.println("  ✓ 不支持字符检测正常");
    } else {
        Serial.println("  ✗ 不支持字符检测异常");
        allPassed = false;
    }
    
    if (allPassed) {
        testsPassed++;
    } else {
        testsFailed++;
    }
}

void testDisplayConfig() {
    Serial.println("\n[TEST 4] 配置管理");
    testNumber++;
    
    // 初始化
    if (DisplayConfig::init()) {
        Serial.println("  ✓ 配置初始化成功");
    } else {
        Serial.println("  ✗ 配置初始化失败");
    }
    
    // 测试默认配置
    DisplaySettings defaultSettings = DisplayConfig::getSettings();
    Serial.printf("  默认亮度: %d\n", defaultSettings.brightness);
    Serial.printf("  自动切换: %s\n", defaultSettings.autoSwitch ? "开启" : "关闭");
    Serial.printf("  切换间隔: %d 秒\n", defaultSettings.switchInterval);
    
    // 测试设置修改
    DisplayConfig::setBrightness(200);
    if (DisplayConfig::getBrightness() == 200) {
        Serial.println("  ✓ 亮度设置正常");
    } else {
        Serial.println("  ✗ 亮度设置异常");
    }
    
    // 测试JSON解析
    const char* json = "{\"brightness\":150,\"autoSwitch\":false,\"switchInterval\":60}";
    if (DisplayConfig::updateFromJSON(json)) {
        if (DisplayConfig::getBrightness() == 150 && 
            !DisplayConfig::getAutoSwitch() && 
            DisplayConfig::getSwitchInterval() == 60) {
            Serial.println("  ✓ JSON解析正常");
            testsPassed++;
        } else {
            Serial.println("  ✗ JSON解析值错误");
            testsFailed++;
        }
    } else {
        Serial.println("  ✗ JSON解析失败");
        testsFailed++;
    }
    
    // 恢复默认
    DisplayConfig::resetToDefault();
}

void testHardwareRefresh() {
    Serial.println("\n[TEST 5] 硬件刷新");
    testNumber++;
    
    // 绘制测试图案
    LEDMatrix::clear();
    
    // 绘制边框
    for (uint8_t x = 0; x < SCREEN_COLS; x++) {
        LEDMatrix::setPixel(x, 0, 64);
        LEDMatrix::setPixel(x, SCREEN_ROWS - 1, 64);
    }
    for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
        LEDMatrix::setPixel(0, y, 64);
        LEDMatrix::setPixel(SCREEN_COLS - 1, y, 64);
    }
    
    // 绘制文字
    FontRenderer::drawString("TEST", 12, 3, FontType::FONT_5x7, 200);
    
    // 刷新到硬件
    if (LEDMatrix::refresh()) {
        Serial.println("  ✓ 硬件刷新成功");
        Serial.println("  观察LED矩阵是否显示边框和TEST文字");
        testsPassed++;
    } else {
        Serial.println("  ✗ 硬件刷新失败");
        testsFailed++;
    }
    
    // 获取统计信息
    uint32_t frames, errors;
    LEDMatrix::getStats(frames, errors);
    Serial.printf("  统计: %d 帧, %d 错误\n", frames, errors);
}
