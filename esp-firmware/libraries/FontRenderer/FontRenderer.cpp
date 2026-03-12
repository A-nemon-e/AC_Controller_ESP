/**
 * FontRenderer - 字体渲染引擎实现
 * 
 * 注意：主要函数已实现为内联函数在头文件中
 * 此文件仅保留非模板辅助函数的实现
 */

#include "FontRenderer.h"

// 字符串渲染实现
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

// 尺寸计算实现
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
        char c = *str++;
        if (c == ' ') {
            width += charWidth;
        } else {
            width += charWidth + 1;  // 字符宽度 + 间距
        }
    }
    
    // 减去最后一个间距
    if (width > 0) {
        width -= 1;
    }
    
    return width;
}

uint8_t FontRenderer::calculateX(const char* str, uint8_t y, FontType font, TextPosition position) {
    uint16_t strWidth = getStringWidth(str, font);
    
    // 根据位置计算X坐标
    switch (position) {
        case TextPosition::TOP_LEFT:
        case TextPosition::CENTER_LEFT:
        case TextPosition::BOTTOM_LEFT:
            return 0;
            
        case TextPosition::TOP_CENTER:
        case TextPosition::CENTER:
        case TextPosition::BOTTOM_CENTER:
            return (SCREEN_COLS - strWidth) / 2;
            
        case TextPosition::TOP_RIGHT:
        case TextPosition::CENTER_RIGHT:
        case TextPosition::BOTTOM_RIGHT:
            return (strWidth <= SCREEN_COLS) ? (SCREEN_COLS - strWidth) : 0;
            
        default:
            return 0;
    }
}

// 字体数据访问函数（保留但未使用）
const uint8_t* FontRenderer::getFontData(FontType font) {
    (void)font;  // 避免未使用参数警告
    return nullptr;
}

uint8_t FontRenderer::getFontDataWidth(FontType font) {
    return getCharWidth(font);
}

uint8_t FontRenderer::getFontDataHeight(FontType font) {
    return getCharHeight(font);
}
