/**
 * FontRenderer - 字体渲染引擎实现
 * 
 * 统一所有字体渲染逻辑，消除重复代码
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
    
    int8_t idx = getCharIndex(c);
    if (idx < 0) return getCharWidth(font);
    
    // 根据字体类型直接绘制
    switch (font) {
        case FontType::FONT_3x5: {
            if (idx >= FONT_3X5_COUNT) return 3;
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
        case FontType::FONT_5x7: {
            if (idx >= FONT_5X7_COUNT) return 5;
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
        case FontType::FONT_3x9: {
            if (idx >= FONT_3X9_COUNT) return 3;
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
        case FontType::FONT_5x5: {
            if (idx >= FONT_5X5_COUNT) return 5;
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
        case FontType::FONT_6x9: {
            if (idx > 9) return 6;  // 6x9只支持数字
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
        default:
            return 0;
    }
}

uint16_t FontRenderer::drawString(const char* str, uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    uint16_t totalWidth = 0;
    uint8_t charWidth = getCharWidth(font);
    
    while (*str) {
        char c = *str++;
        
        if (c == ' ') {
            totalWidth += charWidth;
            x += charWidth;
            continue;
        }
        
        uint8_t width = drawChar(c, x, y, font, brightness);
        totalWidth += width;
        x += width;
        totalWidth += 1;
        x += 1;
    }
    
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
    uint16_t width = 0;
    uint8_t charWidth = getCharWidth(font);
    
    while (*str) {
        if (*str == ' ') {
            width += charWidth;
        } else {
            width += charWidth;
        }
        str++;
        width += 1;  // 字符间距
    }
    
    if (width > 0) width -= 1;  // 减去最后一个间距
    return width;
}

uint8_t FontRenderer::calculateX(const char* str, uint8_t y, FontType font, TextPosition position) {
    uint16_t width = getStringWidth(str, font);
    
    switch (position) {
        case TextPosition::TOP_LEFT:
        case TextPosition::CENTER_LEFT:
        case TextPosition::BOTTOM_LEFT:
            return 0;
        case TextPosition::TOP_CENTER:
        case TextPosition::CENTER:
        case TextPosition::BOTTOM_CENTER:
            return (SCREEN_COLS - width) / 2;
        case TextPosition::TOP_RIGHT:
        case TextPosition::CENTER_RIGHT:
        case TextPosition::BOTTOM_RIGHT:
            return SCREEN_COLS - width;
        default:
            return 0;
    }
}

void FontRenderer::drawColon(uint8_t x, uint8_t y, FontType font, uint8_t brightness) {
    switch (font) {
        case FontType::FONT_3x5:
            drawColonData(x, y, Font3x5_Colon, 3, 5, brightness);
            break;
        case FontType::FONT_5x7:
            drawColonData(x, y, Font5x7_Colon, 5, 7, brightness);
            break;
        case FontType::FONT_3x9:
            drawColonData(x, y, Font3x9_Colon, 3, 9, brightness);
            break;
        case FontType::FONT_5x5:
            drawColonData(x, y, Font5x5_Colon, 5, 5, brightness);
            break;
        case FontType::FONT_6x9:
            drawColonData(x, y, Font6x9_Colon, 6, 9, brightness);
            break;
    }
}

void FontRenderer::drawColonData(uint8_t x, uint8_t y, const uint8_t* data, uint8_t w, uint8_t h, uint8_t brightness) {
    for (uint8_t row = 0; row < h; row++) {
        uint8_t rowData = pgm_read_byte(&data[row]);
        for (uint8_t col = 0; col < w; col++) {
            if (rowData & (1 << (w - 1 - col))) {
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
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        return CHAR_INDEX_LETTER_A + (c - 'A');
    } else if (c == ':') {
        return CHAR_INDEX_COLON;
    } else if (c == ' ') {
        return CHAR_INDEX_SPACE;
    }
    return -1;
}

bool FontRenderer::isCharSupported(char c) {
    return getCharIndex(c) >= 0;
}

const uint8_t* FontRenderer::getFontData(FontType font) {
    switch (font) {
        case FontType::FONT_3x5: return Font3x5[0];
        case FontType::FONT_5x7: return Font5x7[0];
        case FontType::FONT_3x9: return Font3x9[0];
        case FontType::FONT_5x5: return Font5x5[0];
        case FontType::FONT_6x9: return Font6x9[0];
        default: return nullptr;
    }
}

uint8_t FontRenderer::getFontDataWidth(FontType font) {
    return getCharWidth(font);
}

uint8_t FontRenderer::getFontDataHeight(FontType font) {
    return getCharHeight(font);
}
