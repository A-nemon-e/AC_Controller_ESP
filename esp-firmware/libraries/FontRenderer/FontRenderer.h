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
#include "Fonts/Font_3x5.h"
#include "Fonts/Font_5x7.h"
#include "Fonts/Font_3x9.h"
#include "Fonts/Font_5x5.h"
#include "Fonts/Font_6x9.h"

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

// 字符索引常量
#define CHAR_INDEX_DIGIT_0  0
#define CHAR_INDEX_DIGIT_9  9
#define CHAR_INDEX_LETTER_A 10
#define CHAR_INDEX_LETTER_Z 35
#define CHAR_INDEX_COLON    36
#define CHAR_INDEX_SPACE    37

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
    
    // 字体数据访问函数
    static const uint8_t* getFontData(FontType font);
    static uint8_t getFontDataWidth(FontType font);
    static uint8_t getFontDataHeight(FontType font);

private:
    // ==================== 模板渲染函数（内联实现） ====================
    
    // 通用字体渲染模板
    template<uint8_t WIDTH, uint8_t HEIGHT, uint8_t COUNT>
    static uint8_t drawCharTemplate(
        char c, 
        uint8_t x, 
        uint8_t y, 
        uint8_t brightness,
        const uint8_t fontData[COUNT][HEIGHT]
    ) {
        int8_t idx = getCharIndex(c);
        if (idx < 0 || idx >= (int8_t)COUNT) return WIDTH;
        
        for (uint8_t row = 0; row < HEIGHT; row++) {
            uint8_t rowData = pgm_read_byte(&fontData[idx][row]);
            for (uint8_t col = 0; col < WIDTH; col++) {
                if (rowData & (1 << (WIDTH - 1 - col))) {
                    LEDMatrix::setPixelClipped(x + col, y + row, brightness);
                }
            }
        }
        return WIDTH;
    }
    
    // 6x9字体专用渲染（仅支持数字）
    static uint8_t drawChar6x9(char c, uint8_t x, uint8_t y, uint8_t brightness) {
        int8_t idx = getCharIndex(c);
        // 6x9只支持数字0-9
        if (idx < 0 || idx > 9) return 6;
        
        for (uint8_t row = 0; row < 9; row++) {
            uint8_t rowData = pgm_read_byte(&Font6x9[idx][row]);
            for (uint8_t col = 0; col < 6; col++) {
                if (rowData & (1 << (5 - col))) {
                    LEDMatrix::setPixelClipped(x + col, y + row, brightness);
                }
            }
        }
        return 6;
    }
    
    // 冒号渲染模板
    template<uint8_t W, uint8_t H>
    static void drawColonTemplate(uint8_t x, uint8_t y, const uint8_t colonData[H], uint8_t brightness) {
        for (uint8_t row = 0; row < H; row++) {
            uint8_t rowData = pgm_read_byte(&colonData[row]);
            for (uint8_t col = 0; col < W; col++) {
                if (rowData & (1 << (W - 1 - col))) {
                    LEDMatrix::setPixelClipped(x + col, y + row, brightness);
                }
            }
        }
    }
};

// ==================== 内联函数实现 ====================

inline uint8_t FontRenderer::drawChar(char c, uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    // 转换为大写
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }
    
    switch (font) {
        case FontType::FONT_3x5:
            return drawCharTemplate<3, 5, FONT_3X5_COUNT>(c, x, y, brightness, Font3x5);
        case FontType::FONT_5x7:
            return drawCharTemplate<5, 7, FONT_5X7_COUNT>(c, x, y, brightness, Font5x7);
        case FontType::FONT_3x9:
            return drawCharTemplate<3, 9, FONT_3X9_COUNT>(c, x, y, brightness, Font3x9);
        case FontType::FONT_5x5:
            return drawCharTemplate<5, 5, FONT_5X5_COUNT>(c, x, y, brightness, Font5x5);
        case FontType::FONT_6x9:
            return drawChar6x9(c, x, y, brightness);
        default:
            return 0;
    }
}

inline void FontRenderer::drawColon(uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    switch (font) {
        case FontType::FONT_3x5:
            drawColonTemplate<3, 5>(x, y, Font3x5_Colon, brightness);
            break;
        case FontType::FONT_5x7:
            drawColonTemplate<5, 7>(x, y, Font5x7_Colon, brightness);
            break;
        case FontType::FONT_3x9:
            drawColonTemplate<3, 9>(x, y, Font3x9_Colon, brightness);
            break;
        case FontType::FONT_5x5:
            drawColonTemplate<5, 5>(x, y, Font5x5_Colon, brightness);
            break;
        case FontType::FONT_6x9:
            drawColonTemplate<6, 9>(x, y, Font6x9_Colon, brightness);
            break;
    }
}

inline void FontRenderer::drawIcon(const uint8_t* iconData, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness) {
    for (uint8_t row = 0; row < h; row++) {
        uint8_t rowData = pgm_read_byte(&iconData[row]);
        for (uint8_t col = 0; col < w; col++) {
            if (rowData & (1 << (w - 1 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

inline int8_t FontRenderer::getCharIndex(char c) {
    // 数字 0-9
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    
    // 字母 A-Z
    if (c >= 'A' && c <= 'Z') {
        return 10 + (c - 'A');
    }
    
    // 冒号
    if (c == ':') {
        return CHAR_INDEX_COLON;
    }
    
    // 空格
    if (c == ' ') {
        return CHAR_INDEX_SPACE;
    }
    
    // 不支持
    return -1;
}

inline bool FontRenderer::isCharSupported(char c) {
    return getCharIndex(c) >= 0;
}

#endif // FONT_RENDERER_H
