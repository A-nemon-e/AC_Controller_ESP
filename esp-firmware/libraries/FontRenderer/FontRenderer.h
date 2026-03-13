/**
 * FontRenderer - 字体渲染引擎
 * 
 * 统一所有字体渲染逻辑，消除重复代码
 * 支持多种字体：3x5, 5x7, 3x9, 5x5, 6x9
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <Arduino.h>
#include "../LEDMatrix/LEDMatrix.h"

// 字体类型枚举
enum class FontType {
    FONT_3x5,   // 3列 x 5行，紧凑
    FONT_5x7,   // 5列 x 7行，标准
    FONT_3x9,   // 3列 x 9行，高窄
    FONT_5x5,   // 5列 x 5行，方形
    FONT_6x9    // 6列 x 9行，大号数字
};

// 位置枚举
enum class TextPosition {
    TOP_LEFT,       // 左上
    TOP_CENTER,     // 上中
    TOP_RIGHT,      // 右上
    CENTER_LEFT,    // 左中
    CENTER,         // 中央
    CENTER_RIGHT,   // 右中
    BOTTOM_LEFT,    // 左下
    BOTTOM_CENTER,  // 下中
    BOTTOM_RIGHT    // 右下
};

class FontRenderer {
public:
    // ==================== 字符渲染 ====================
    
    /**
     * @brief 渲染单个字符
     * @param c 字符（支持0-9, A-Z, 冒号）
     * @param x 起始X坐标
     * @param y 起始Y坐标
     * @param font 字体类型
     * @param brightness 亮度值 0-255
     * @return 渲染后的宽度（可用于计算下一个字符位置）
     */
    static uint8_t drawChar(char c, uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    /**
     * @brief 渲染字符串
     * @param str 字符串
     * @param x 起始X坐标
     * @param y 起始Y坐标
     * @param font 字体类型
     * @param brightness 亮度值
     * @return 渲染后的总宽度
     */
    static uint16_t drawString(const char* str, uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    /**
     * @brief 渲染居中的字符串
     * @param str 字符串
     * @param y Y坐标
     * @param font 字体类型
     * @param brightness 亮度值
     */
    static void drawStringCentered(const char* str, uint8_t y, FontType font, uint8_t brightness = 255);
    
    /**
     * @brief 渲染右对齐的字符串
     * @param str 字符串
     * @param x 右边界X坐标
     * @param y Y坐标
     * @param font 字体类型
     * @param brightness 亮度值
     */
    static void drawStringRight(const char* str, uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    // ==================== 尺寸计算 ====================
    
    /**
     * @brief 获取字符宽度
     * @param font 字体类型
     * @return 字符宽度（像素）
     */
    static uint8_t getCharWidth(FontType font);
    
    /**
     * @brief 获取字符高度
     * @param font 字体类型
     * @return 字符高度（像素）
     */
    static uint8_t getCharHeight(FontType font);
    
    /**
     * @brief 获取字符串总宽度
     * @param str 字符串
     * @param font 字体类型
     * @return 总宽度（像素）
     */
    static uint16_t getStringWidth(const char* str, FontType font);
    
    /**
     * @brief 根据位置计算起始坐标
     * @param str 字符串
     * @param y Y坐标（可以为0，使用默认位置）
     * @param font 字体类型
     * @param position 位置枚举
     * @return 计算后的X坐标
     */
    static uint8_t calculateX(const char* str, uint8_t y, FontType font, TextPosition position);
    
    // ==================== 特殊符号 ====================
    
    /**
     * @brief 渲染冒号
     * @param x X坐标
     * @param y Y坐标
     * @param font 字体类型
     * @param brightness 亮度值
     */
    static void drawColon(uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    /**
     * @brief 渲染冒号数据
     */
    static void drawColonData(uint8_t x, uint8_t y, const uint8_t* data, uint8_t w, uint8_t h, uint8_t brightness);
    
    /**
     * @brief 渲染图标
     * @param iconData 图标数据指针
     * @param x X坐标
     * @param y Y坐标
     * @param w 图标宽度
     * @param h 图标高度
     * @param brightness 亮度值
     */
    static void drawIcon(const uint8_t* iconData, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness = 255);
    
    // ==================== 工具方法 ====================
    
    /**
     * @brief 获取字符在字体数组中的索引
     * @param c 字符
     * @return 索引，不支持则返回-1
     */
    static int8_t getCharIndex(char c);
    
    /**
     * @brief 检查字符是否支持
     * @param c 字符
     * @return true支持，false不支持
     */
    static bool isCharSupported(char c);

private:
    // 获取字体数据指针
    static const uint8_t* getFontData(FontType font);
    static uint8_t getFontDataWidth(FontType font);
    static uint8_t getFontDataHeight(FontType font);
};

#endif // FONT_RENDERER_H
