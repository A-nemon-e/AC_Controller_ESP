/**
 * TempForeground - 温湿度前景类
 * 
 * 显示温度和湿度
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef TEMP_FOREGROUND_H
#define TEMP_FOREGROUND_H

#include "../Foregrounds/Foreground.h"

class TempForeground : public Foreground {
public:
    enum class Format {
        TEMP_ONLY,      // 仅温度
        TEMP_HUMID,     // 温度+湿度
        HUMID_ONLY      // 仅湿度
    };
    
    TempForeground() : Foreground(Type::TEMP_HUMID), format(Format::TEMP_HUMID),
                      temperature(25.0f), humidity(60.0f) {
        setFont(FontType::FONT_3x5);
    }
    
    void setFormat(Format fmt) { format = fmt; }
    Format getFormat() const { return format; }
    
    void setTemperature(float temp) { temperature = temp; }
    void setHumidity(float humid) { humidity = humid; }
    
    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    
    void render(LEDMatrix& matrix, FontRenderer& fonts) override {
        char buffer[20];
        uint8_t contentW = 0, contentH = 0;
        
        switch (format) {
            case Format::TEMP_ONLY:
                snprintf(buffer, sizeof(buffer), "%02dC", (int)temperature);
                contentW = fonts.getStringWidth(buffer, font);
                contentH = fonts.getCharHeight(font);
                break;
                
            case Format::TEMP_HUMID:
                snprintf(buffer, sizeof(buffer), "%02dC %02d%%", 
                        (int)temperature, (int)humidity);
                contentW = fonts.getStringWidth(buffer, font);
                contentH = fonts.getCharHeight(font);
                break;
                
            case Format::HUMID_ONLY:
                snprintf(buffer, sizeof(buffer), "%02d%%", (int)humidity);
                contentW = fonts.getStringWidth(buffer, font);
                contentH = fonts.getCharHeight(font);
                break;
        }
        
        // 计算位置
        uint8_t x, y;
        calculatePosition(x, y, contentW, contentH);
        
        // 渲染
        fonts.drawString(buffer, x, y, font, brightness);
    }
    
    void getSize(uint8_t& w, uint8_t& h) const override {
        switch (format) {
            case Format::TEMP_ONLY:
                w = 3 * FontRenderer::getCharWidth(font);  // XXC
                break;
            case Format::TEMP_HUMID:
                w = 7 * FontRenderer::getCharWidth(font);  // XXC XX%
                break;
            case Format::HUMID_ONLY:
                w = 3 * FontRenderer::getCharWidth(font);  // XX%
                break;
        }
        h = FontRenderer::getCharHeight(font);
    }

private:
    Format format;
    float temperature;
    float humidity;
};

#endif // TEMP_FOREGROUND_H
