/**
 * LEDMatrix - LED矩阵Framebuffer封装
 * 
 * 基于IS31FL3733驱动的硬件抽象层
 * 提供统一的Framebuffer操作接口
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <Arduino.h>
#include <Wire.h>
#include <IS31FL3733.h>

// 屏幕尺寸定义
#define SCREEN_ROWS 11
#define SCREEN_COLS 42
#define NUM_CHIPS 6

// 错误码定义
enum class LEDMatrixError {
    NONE = 0,
    OUT_OF_BOUNDS,      // 坐标越界
    I2C_ERROR,          // I2C通信错误
    NOT_INITIALIZED,    // 未初始化
    BUFFER_OVERFLOW     // 缓冲区溢出
};

class LEDMatrix {
public:
    // ==================== 初始化与生命周期 ====================
    
    /**
     * @brief 初始化LED矩阵系统
     * @return true成功，false失败
     */
    static bool begin();
    
    /**
     * @brief 关闭LED矩阵，释放资源
     */
    static void end();
    
    /**
     * @brief 检查是否已初始化
     */
    static bool isInitialized() { return initialized; }
    
    // ==================== 缓冲区操作 ====================
    
    /**
     * @brief 清空整个缓冲区
     */
    static void clear();
    
    /**
     * @brief 填充整个缓冲区为指定亮度
     * @param brightness 亮度值 0-255
     */
    static void fill(uint8_t brightness);
    
    /**
     * @brief 填充矩形区域
     * @param x 起始X坐标
     * @param y 起始Y坐标
     * @param w 宽度
     * @param h 高度
     * @param brightness 亮度值
     */
    static void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness);
    
    // ==================== 像素操作 ====================
    
    /**
     * @brief 设置指定坐标的像素亮度
     * @param x X坐标 (0-41)
     * @param y Y坐标 (0-10)
     * @param brightness 亮度值 (0-255)
     * @return true成功，false越界
     */
    static bool setPixel(uint8_t x, uint8_t y, uint8_t brightness);
    
    /**
     * @brief 获取指定坐标的像素亮度
     * @param x X坐标
     * @param y Y坐标
     * @return 亮度值，越界返回0
     */
    static uint8_t getPixel(uint8_t x, uint8_t y);
    
    /**
     * @brief 安全设置像素（带裁剪）
     * @param x X坐标（可为负或超出范围）
     * @param y Y坐标（可为负或超出范围）
     * @param brightness 亮度值
     * @return true像素被设置，false被裁剪
     */
    static bool setPixelClipped(int16_t x, int16_t y, uint8_t brightness);
    
    // ==================== 渲染输出 ====================
    
    /**
     * @brief 将缓冲区内容刷新到硬件
     * @return true成功，false失败
     */
    static bool refresh();
    
    /**
     * @brief 将另一个缓冲区的内容复制到当前缓冲区
     * @param source 源缓冲区
     */
    static void copyFrom(const uint8_t source[SCREEN_ROWS][SCREEN_COLS]);
    
    /**
     * @brief 获取缓冲区指针（用于直接操作）
     * @return 缓冲区指针
     */
    static uint8_t (*getBuffer())[SCREEN_COLS] { return buffer; }
    
    // ==================== 亮度控制 ====================
    
    /**
     * @brief 设置全局亮度（GCC寄存器）
     * @param gcc 全局亮度值 0-255
     */
    static void setGlobalBrightness(uint8_t gcc);
    
    /**
     * @brief 获取当前全局亮度
     */
    static uint8_t getGlobalBrightness() { return globalBrightness; }
    
    /**
     * @brief 开关屏幕
     * @param on true开，false关
     */
    static void setScreenOn(bool on);
    
    /**
     * @brief 检查屏幕是否开启
     */
    static bool isScreenOn() { return screenOn; }
    
    // ==================== 错误处理 ====================
    
    /**
     * @brief 获取最后错误码
     */
    static LEDMatrixError getLastError() { return lastError; }
    
    /**
     * @brief 获取错误描述
     * @param error 错误码
     * @return 错误描述字符串
     */
    static const char* getErrorString(LEDMatrixError error);
    
    /**
     * @brief 清除错误状态
     */
    static void clearError() { lastError = LEDMatrixError::NONE; }
    
    // ==================== 调试工具 ====================
    
    /**
     * @brief 测试模式：逐个点亮所有LED
     */
    static void testPattern();
    
    /**
     * @brief 扫描I2C总线并打印结果
     */
    static void scanI2C();
    
    /**
     * @brief 获取统计信息
     * @param frameCount 输出的帧数
     * @param errorCount 输出的错误数
     */
    static void getStats(uint32_t& frameCount, uint32_t& errorCount);

private:
    // 缓冲区 - 42列 x 11行
    static uint8_t buffer[SCREEN_ROWS][SCREEN_COLS];
    
    // IS31FL3733驱动实例
    static IS31FL3733::IS31FL3733Driver* drivers[NUM_CHIPS];
    
    // 状态
    static bool initialized;
    static bool screenOn;
    static uint8_t globalBrightness;
    static LEDMatrixError lastError;
    
    // 统计
    static uint32_t frameCounter;
    static uint32_t errorCounter;
    
    // 内部方法
    static void setError(LEDMatrixError error);
    static bool isValidPosition(uint8_t x, uint8_t y);
    static void bufferToPWM(uint8_t chipIndex, uint8_t* pwmBuffer);
    
    // I2C回调函数
    static uint8_t i2cWriteReg(uint8_t addr, uint8_t reg, const uint8_t* buf, uint8_t cnt);
    static uint8_t i2cReadReg(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t cnt);
};

#endif // LED_MATRIX_H
