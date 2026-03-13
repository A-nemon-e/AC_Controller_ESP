/**
 * ClockForeground - 时钟前景类
 * 
 * 显示当前时间
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef CLOCK_FOREGROUND_H
#define CLOCK_FOREGROUND_H

#include "../Foregrounds/Foreground.h"
#include <time.h>

class ClockForeground : public Foreground {
public:
    enum class Format {
        HH_MM,      // 时:分
        HH_MM_SS    // 时:分:秒
    };
    
    ClockForeground() : Foreground(Type::CLOCK), format(Format::HH_MM) {
        setFont(FontType::FONT_5x7);
    }
    
    void setFormat(Format fmt) { format = fmt; }
    Format getFormat() const { return format; }
    
    void render(LEDMatrix& matrix, FontRenderer& fonts) override {
        // 获取当前时间（从NTP）
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        uint8_t hour = timeinfo ? timeinfo->tm_hour : 0;
        uint8_t minute = timeinfo ? timeinfo->tm_min : 0;
        uint8_t second = timeinfo ? timeinfo->tm_sec : 0;
        
        char buffer[10];
        uint8_t contentW = 0, contentH = 0;
        
        if (format == Format::HH_MM) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
            contentW = fonts.getStringWidth(buffer, font);
            contentH = fonts.getCharHeight(font);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour, minute, second);
            contentW = fonts.getStringWidth(buffer, font);
            contentH = fonts.getCharHeight(font);
        }
        
        // 计算位置
        uint8_t x, y;
        calculatePosition(x, y, contentW, contentH);
        
        // 渲染
        fonts.drawString(buffer, x, y, font, brightness);
    }
    
    void getSize(uint8_t& w, uint8_t& h) const override {
        if (format == Format::HH_MM) {
            // HH:MM = 5 chars
            w = 5 * FontRenderer::getCharWidth(font);
            h = FontRenderer::getCharHeight(font);
        } else {
            // HH:MM:SS = 8 chars
            w = 8 * FontRenderer::getCharWidth(font);
            h = FontRenderer::getCharHeight(font);
        }
    }

private:
    Format format;
};

#endif // CLOCK_FOREGROUND_H
