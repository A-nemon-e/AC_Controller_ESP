/**
 * DisplayConfig - 显示配置管理
 * 
 * 管理显示设置（亮度、卡片、自动切换等）
 * 支持EEPROM持久化和MQTT配置更新
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../LEDMatrix/config_pins.h"

// 注意：TransitionType 定义在 Transition.h 中，这里不再重复定义

// 显示设置数据结构（与EEPROM布局对应）
struct DisplaySettings {
    uint8_t brightness;              // 全局亮度 0-255
    uint8_t currentCard;             // 当前卡片索引
    bool autoSwitch;                 // 是否自动切换卡片
    uint16_t switchInterval;         // 切换间隔（秒）
    uint8_t transitionType;          // 转场效果类型
    
    // 构造函数设置默认值
    DisplaySettings() : 
        brightness(128),
        currentCard(0),
        autoSwitch(true),
        switchInterval(30),
        transitionType(1)  // 1 = FADE
    {}
};

class DisplayConfig {
public:
    // ==================== 初始化 ====================
    
    /**
     * @brief 初始化配置管理器（从EEPROM加载）
     * @return true成功，false使用默认配置
     */
    static bool init();
    
    /**
     * @brief 检查是否已初始化
     */
    static bool isInitialized() { return initialized; }
    
    // ==================== 获取配置 ====================
    
    /**
     * @brief 获取完整配置
     */
    static DisplaySettings getSettings() { return settings; }
    
    /**
     * @brief 获取全局亮度
     */
    static uint8_t getBrightness() { return settings.brightness; }
    
    /**
     * @brief 获取当前卡片索引
     */
    static uint8_t getCurrentCard() { return settings.currentCard; }
    
    /**
     * @brief 获取自动切换状态
     */
    static bool getAutoSwitch() { return settings.autoSwitch; }
    
    /**
     * @brief 获取切换间隔（秒）
     */
    static uint16_t getSwitchInterval() { return settings.switchInterval; }
    
    /**
     * @brief 获取转场效果类型
     * @return 转场效果类型 (0-5)
     */
    static uint8_t getTransitionType() { 
        return settings.transitionType; 
    }
    
    // ==================== 设置配置 ====================
    
    /**
     * @brief 设置完整配置
     * @param newSettings 新配置
     */
    static void setSettings(const DisplaySettings& newSettings);
    
    /**
     * @brief 设置全局亮度
     * @param brightness 亮度值 0-255
     */
    static void setBrightness(uint8_t brightness);
    
    /**
     * @brief 设置当前卡片
     * @param cardIndex 卡片索引
     */
    static void setCurrentCard(uint8_t cardIndex);
    
    /**
     * @brief 设置自动切换
     * @param enable true启用，false禁用
     */
    static void setAutoSwitch(bool enable);
    
    /**
     * @brief 设置切换间隔
     * @param seconds 间隔秒数
     */
    static void setSwitchInterval(uint16_t seconds);
    
    /**
     * @brief 设置转场效果
     * @param type 转场类型 (0-5)
     */
    static void setTransitionType(uint8_t type);
    
    // ==================== 持久化 ====================
    
    /**
     * @brief 立即保存到EEPROM
     */
    static void save();
    
    /**
     * @brief 延迟保存（避免频繁写入）
     * 在loop中定期调用flush()来实际保存
     */
    static void markDirty();
    
    /**
     * @brief 检查并执行延迟保存
     * 应在主循环中定期调用
     */
    static void flush();
    
    // ==================== MQTT配置更新 ====================
    
    /**
     * @brief 从JSON字符串更新配置
     * @param json JSON字符串
     * @return true解析成功，false解析失败
     */
    static bool updateFromJSON(const char* json);
    
    /**
     * @brief 将配置导出为JSON字符串
     * @param buffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     * @return 实际写入的长度
     */
    static size_t toJSON(char* buffer, size_t bufferSize);
    
    // ==================== 重置 ====================
    
    /**
     * @brief 重置为默认配置
     */
    static void resetToDefault();
    
    /**
     * @brief 清除EEPROM中的所有配置
     */
    static void clearEEPROM();
    
    // ==================== 调试 ====================
    
    /**
     * @brief 打印当前配置到串口
     */
    static void printConfig();
    
    /**
     * @brief 获取EEPROM校验和（用于调试）
     */
    static uint16_t getChecksum();

private:
    static DisplaySettings settings;
    static bool initialized;
    static bool dirty;
    static unsigned long lastDirtyTime;
    static const unsigned long SAVE_DELAY = 5000;  // 5秒后保存
    
    // EEPROM操作
    static void loadFromEEPROM();
    static void saveToEEPROM();
    static uint16_t calculateChecksum(const DisplaySettings& data);
    static bool validateData(const DisplaySettings& data);
    
    // 默认配置
    static void setDefaults();
};

#endif // DISPLAY_CONFIG_H
