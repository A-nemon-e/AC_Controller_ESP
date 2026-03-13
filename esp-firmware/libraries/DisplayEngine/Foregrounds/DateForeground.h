/**
 * DateForeground - 日期前景类
 * 
 * 显示当前日期
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef DATE_FOREGROUND_H
#define DATE_FOREGROUND_H

#include "../Foregrounds/Foreground.h"

class DateForeground : public Foreground {
public:
    enum class Format {
        MM_DD,      // 月-日
        MM_DD_WEEK  // 月-日 星期
    };
    
    DateForeground() : Foreground(Type::DATE), format(Format::MM_DD) {
        setFont(FontType::FONT_3x5);
    }
    
    void setFormat(Format fmt) { format = fmt; }
    Format getFormat() const { return format; }
    
    void render(LEDMatrix& matrix, FontRenderer& fonts) override {
        // 模拟日期数据
        uint8_t month = 3;
        uint8_t day = 15;
        uint8_t weekday = 6;  // 0=周日, 6=周六
        
        char buffer[16];
        uint8_t contentW = 0, contentH = 0;
        
        if (format == Format::MM_DD) {
            snprintf(buffer, sizeof(buffer), "%02d-%02d", month, day);
            contentW = fonts.getStringWidth(buffer, font);
            contentH = fonts.getCharHeight(font);
        } else {
            const char* weekDays[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
            snprintf(buffer, sizeof(buffer), "%02d-%02d %s", month, day, weekDays[weekday]);
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
        if (format == Format::MM_DD) {
            w = 5 * FontRenderer::getCharWidth(font);  // MM-DD
            h = FontRenderer::getCharHeight(font);
        } else {
            w = 9 * FontRenderer::getCharWidth(font);  // MM-DD WEE
            h = FontRenderer::getCharHeight(font);
        }
    }

private:
    Format format;
};

#endif // DATE_FOREGROUND_H
