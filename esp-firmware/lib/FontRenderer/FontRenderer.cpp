/**
 * FontRenderer - 字体渲染引擎实现
 * 
 * 统一所有字体渲染逻辑，消除重复代码
 * 使用switch-case替代模板，避免编译问题
 */

#include "FontRenderer.h"
#include "Fonts/Font_3x5.h"
#include "Fonts/Font_5x7.h"
#include "Fonts/Font_3x9.h"
#include "Fonts/Font_5x5.h"
#include "Fonts/Font_6x9.h"

// 字符索引常量
#define CHAR_INDEX_DIGIT_0  0
#define CHAR_INDEX_DIGIT_9  9
#define CHAR_INDEX_LETTER_A 10
#define CHAR_INDEX_LETTER_Z 35
#define CHAR_INDEX_COLON    36
#define CHAR_INDEX_SPACE    37

uint8_t FontRenderer::drawChar(char c, uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    // 转换为大写
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }
    
    // 根据字体类型直接渲染
    switch (font) {
        case FontType::FONT_3x5:
            return drawChar3x5(c, x, y, brightness);
        case FontType::FONT_5x7:
            return drawChar5x7(c, x, y, brightness);
        case FontType::FONT_3x9:
            return drawChar3x9(c, x, y, brightness);
        case FontType::FONT_5x5:
            return drawChar5x5(c, x, y, brightness);
        case FontType::FONT_6x9:
            return drawChar6x9(c, x, y, brightness);
        default:
            return 0;
    }
}

// 3x5字体渲染
uint8_t FontRenderer::drawChar3x5(char c, uint8_t x, uint8_t y, uint8_t brightness) {
    int8_t idx = getCharIndex(c);
    if (idx < 0 || idx >= FONT_3X5_COUNT) return 3;
    
    for (uint8_t row = 0; row < 5; row++) {
        uint8_t rowData = pgm_read_byte(&Font3x5[idx][row]);
        for (uint8_t col = 0; col < 3; col++) {
            if (rowData & (1 << (2 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
    return 3;
}

// 5x7字体渲染
uint8_t FontRenderer::drawChar5x7(char c, uint8_t x, uint8_t y, uint8_t brightness) {
    int8_t idx = getCharIndex(c);
    if (idx < 0 || idx >= FONT_5X7_COUNT) return 5;
    
    for (uint8_t row = 0; row < 7; row++) {
        uint8_t rowData = pgm_read_byte(&Font5x7[idx][row]);
        for (uint8_t col = 0; col < 5; col++) {
            if (rowData & (1 << (4 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
    return 5;
}

// 3x9字体渲染
uint8_t FontRenderer::drawChar3x9(char c, uint8_t x, uint8_t y, uint8_t brightness) {
    int8_t idx = getCharIndex(c);
    if (idx < 0 || idx >= FONT_3X9_COUNT) return 3;
    
    for (uint8_t row = 0; row < 9; row++) {
        uint8_t rowData = pgm_read_byte(&Font3x9[idx][row]);
        for (uint8_t col = 0; col < 3; col++) {
            if (rowData & (1 << (2 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
    return 3;
}

// 5x5字体渲染
uint8_t FontRenderer::drawChar5x5(char c, uint8_t x, uint8_t y, uint8_t brightness) {
    int8_t idx = getCharIndex(c);
    if (idx < 0 || idx >= FONT_5X5_COUNT) return 5;
    
    for (uint8_t row = 0; row < 5; row++) {
        uint8_t rowData = pgm_read_byte(&Font5x5[idx][row]);
        for (uint8_t col = 0; col < 5; col++) {
            if (rowData & (1 << (4 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
    return 5;
}

// 6x9字体渲染
uint8_t FontRenderer::drawChar6x9(char c, uint8_t x, uint8_t y, uint8_t brightness) {
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

uint16_t FontRenderer::drawString(const char* str, uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    uint16_t totalWidth = 0;
    uint8_t charWidth = getCharWidth(font);
    
    while (*str) {
        char c = *str++;
        
        if (c == ' ') {
            // 空格只增加宽度
            totalWidth += charWidth;
            x += charWidth;
            continue;
        }
        
        // 绘制字符
        uint8_t width = drawChar(c, x, y, font, brightness);
        totalWidth += width;
        x += width;
        
        // 添加字符间距（1像素）
        totalWidth += 1;
        x += 1;
    }
    
    // 减去最后一个间距
    if (totalWidth > 0) {
        totalWidth -= 1;
    }
    
    return totalWidth;
}

void FontRenderer::drawStringCentered(const char* str, uint8_t y, FontType font, uint8_t brightness) {
    uint16_t width = getStringWidth(str, font);
    uint8_t x = (SCREEN_COLS - width) / 2;
    drawString(str, x, y, font, brightness);
}

void FontRenderer::drawStringRight(const char* str, uint8_t rightX, uint8_t y, FontType font, uint8_t brightness) {
    uint16_t width = getStringWidth(str, font);
    uint8_t x = (rightX >= width) ? (rightX - width) : 0;
    drawString(str, x, y, font, brightness);
}

uint8_t FontRenderer::getCharWidth(FontType font) {
    switch (font) {
        case FontType::FONT_3x5:  return 3;
        case FontType::FONT_5x7:  return 5;
        case FontType::FONT_3x9:  return 3;
        case FontType::FONT_5x5:  return 5;
        case FontType::FONT_6x9:  return 6;
        default: return 5;
    }
}

uint8_t FontRenderer::getCharHeight(FontType font) {
    switch (font) {
        case FontType::FONT_3x5:  return 5;
        case FontType::FONT_5x7:  return 7;
        case FontType::FONT_3x9:  return 9;
        case FontType::FONT_5x5:  return 5;
        case FontType::FONT_6x9:  return 9;
        default: return 7;
    }
}

uint16_t FontRenderer::getStringWidth(const char* str, FontType font) {
    uint8_t charWidth = getCharWidth(font);
    uint16_t width = 0;
    
    while (*str) {
        if (*str == ' ') {
            width += charWidth;
        } else {
            width += charWidth;
        }
        str++;
        
        // 添加字符间距
        if (*str) {
            width += 1;
        }
    }
    
    return width;
}

uint8_t FontRenderer::calculateX(const char* str, uint8_t y, FontType font, TextPosition position) {
    uint16_t strWidth = getStringWidth(str, font);
    uint8_t charHeight = getCharHeight(font);
    
    // 计算X坐标
    uint8_t x = 0;
    switch (position) {
        case TextPosition::TOP_LEFT:
        case TextPosition::CENTER_LEFT:
        case TextPosition::BOTTOM_LEFT:
            x = 0;
            break;
            
        case TextPosition::TOP_CENTER:
        case TextPosition::CENTER:
        case TextPosition::BOTTOM_CENTER:
            x = (SCREEN_COLS - strWidth) / 2;
            break;
            
        case TextPosition::TOP_RIGHT:
        case TextPosition::CENTER_RIGHT:
        case TextPosition::BOTTOM_RIGHT:
            x = (SCREEN_COLS > strWidth) ? (SCREEN_COLS - strWidth) : 0;
            break;
    }
    
    return x;
}

void FontRenderer::drawColon(uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    switch (font) {
        case FontType::FONT_3x5:
            drawColon3x5(x, y, brightness);
            break;
        case FontType::FONT_5x7:
            drawColon5x7(x, y, brightness);
            break;
        case FontType::FONT_3x9:
            drawColon3x9(x, y, brightness);
            break;
        case FontType::FONT_5x5:
            drawColon5x5(x, y, brightness);
            break;
        case FontType::FONT_6x9:
            drawColon6x9(x, y, brightness);
            break;
    }
}

// 3x5冒号
void FontRenderer::drawColon3x5(uint8_t x, uint8_t y, uint8_t brightness) {
    const uint8_t colonData[5] = {0x00, 0x02, 0x00, 0x02, 0x00};
    for (uint8_t row = 0; row < 5; row++) {
        uint8_t rowData = colonData[row];
        for (uint8_t col = 0; col < 3; col++) {
            if (rowData & (1 << (2 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

// 5x7冒号
void FontRenderer::drawColon5x7(uint8_t x, uint8_t y, uint8_t brightness) {
    const uint8_t colonData[7] = {0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00};
    for (uint8_t row = 0; row < 7; row++) {
        uint8_t rowData = colonData[row];
        for (uint8_t col = 0; col < 5; col++) {
            if (rowData & (1 << (4 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

// 3x9冒号
void FontRenderer::drawColon3x9(uint8_t x, uint8_t y, uint8_t brightness) {
    const uint8_t colonData[9] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};
    for (uint8_t row = 0; row < 9; row++) {
        uint8_t rowData = colonData[row];
        for (uint8_t col = 0; col < 3; col++) {
            if (rowData & (1 << (2 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

// 5x5冒号
void FontRenderer::drawColon5x5(uint8_t x, uint8_t y, uint8_t brightness) {
    const uint8_t colonData[5] = {0x00, 0x04, 0x00, 0x04, 0x00};
    for (uint8_t row = 0; row < 5; row++) {
        uint8_t rowData = colonData[row];
        for (uint8_t col = 0; col < 5; col++) {
            if (rowData & (1 << (4 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

// 6x9冒号
void FontRenderer::drawColon6x9(uint8_t x, uint8_t y, uint8_t brightness) {
    const uint8_t colonData[9] = {0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00};
    for (uint8_t row = 0; row < 9; row++) {
        uint8_t rowData = colonData[row];
        for (uint8_t col = 0; col < 6; col++) {
            if (rowData & (1 << (5 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

void FontRenderer::drawIcon(const uint8_t* iconData, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness) {
    for (uint8_t row = 0; row < h; row++) {
        uint8_t rowData = pgm_read_byte(&iconData[row]);
        for (uint8_t col = 0; col < w; col++) {
            if (rowData & (1 << (w - 1 - col))) {
                LEDMatrix::setPixelClipped(x + col, y + row, brightness);
            }
        }
    }
}

int8_t FontRenderer::getCharIndex(char c) {
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
    
    // 不支持的字符
    return -1;
}

bool FontRenderer::isCharSupported(char c) {
    return getCharIndex(c) >= 0;
}