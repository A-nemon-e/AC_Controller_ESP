/**
 * DisplayConfig - 显示配置管理实现 (ArduinoJson优化版)
 * 
 * 集成ArduinoJson v6.x库进行JSON解析和生成
 * 优势：
 * - 更强大的JSON解析能力
 * - 更好的错误处理
 * - 支持嵌套对象和数组
 * - 类型安全
 * 
 * @author AI Assistant
 * @version 1.1.0
 */

#include "DisplayConfig.h"
#include <ArduinoJson.h>

// 静态成员初始化
DisplaySettings DisplayConfig::settings;
bool DisplayConfig::initialized = false;
bool DisplayConfig::dirty = false;
unsigned long DisplayConfig::lastDirtyTime = 0;

bool DisplayConfig::init() {
    // 初始化EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // 尝试从EEPROM加载
    loadFromEEPROM();
    
    initialized = true;
    dirty = false;
    
    return true;
}

void DisplayConfig::setSettings(const DisplaySettings& newSettings) {
    settings = newSettings;
    markDirty();
}

void DisplayConfig::setBrightness(uint8_t brightness) {
    settings.brightness = brightness;
    markDirty();
}

void DisplayConfig::setCurrentCard(uint8_t cardIndex) {
    settings.currentCard = cardIndex;
    markDirty();
}

void DisplayConfig::setAutoSwitch(bool enable) {
    settings.autoSwitch = enable;
    markDirty();
}

void DisplayConfig::setSwitchInterval(uint16_t seconds) {
    settings.switchInterval = seconds;
    markDirty();
}

void DisplayConfig::setTransitionType(uint8_t type) {
    settings.transitionType = type;
    markDirty();
}

void DisplayConfig::save() {
    if (dirty) {
        saveToEEPROM();
        dirty = false;
    }
}

void DisplayConfig::markDirty() {
    dirty = true;
    lastDirtyTime = millis();
}

void DisplayConfig::flush() {
    if (dirty && (millis() - lastDirtyTime > SAVE_DELAY)) {
        save();
    }
}

/**
 * ArduinoJson优化的JSON解析
 * 支持更复杂的JSON结构和类型检查
 */
bool DisplayConfig::updateFromJSON(const char* json) {
    StaticJsonDocument<512> doc;  // 512字节静态缓冲区，避免堆分配
    
    // 解析JSON
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print(F("[DisplayConfig] JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }
    
    bool hasChanges = false;
    
    // 解析亮度 (类型检查)
    if (doc.containsKey("brightness")) {
        int val = doc["brightness"].as<int>();
        if (val >= 0 && val <= 255) {
            settings.brightness = static_cast<uint8_t>(val);
            hasChanges = true;
        } else {
            Serial.println(F("[DisplayConfig] Warning: brightness out of range"));
        }
    }
    
    // 解析自动切换
    if (doc.containsKey("autoSwitch")) {
        settings.autoSwitch = doc["autoSwitch"].as<bool>();
        hasChanges = true;
    }
    
    // 解析切换间隔
    if (doc.containsKey("switchInterval")) {
        int val = doc["switchInterval"].as<int>();
        if (val > 0 && val < 3600) {
            settings.switchInterval = static_cast<uint16_t>(val);
            hasChanges = true;
        } else {
            Serial.println(F("[DisplayConfig] Warning: switchInterval out of range"));
        }
    }
    
    // 解析转场类型
    if (doc.containsKey("transitionType")) {
        int val = doc["transitionType"].as<int>();
        if (val >= 0 && val <= 5) {
            settings.transitionType = static_cast<uint8_t>(val);
            hasChanges = true;
        } else {
            Serial.println(F("[DisplayConfig] Warning: transitionType out of range"));
        }
    }
    
    // 解析当前卡片
    if (doc.containsKey("currentCard")) {
        int val = doc["currentCard"].as<int>();
        if (val >= 0 && val < 10) {  // 假设最多10个卡片
            settings.currentCard = static_cast<uint8_t>(val);
            hasChanges = true;
        } else {
            Serial.println(F("[DisplayConfig] Warning: currentCard out of range"));
        }
    }
    
    // 如果解析成功且有变化，标记为dirty
    if (hasChanges) {
        markDirty();
        Serial.println(F("[DisplayConfig] Settings updated from JSON"));
    }
    
    return true;
}

/**
 * ArduinoJson优化的JSON生成
 * 生成格式化JSON，支持扩展字段
 */
size_t DisplayConfig::toJSON(char* buffer, size_t bufferSize) {
    StaticJsonDocument<512> doc;
    
    // 添加配置字段
    doc["brightness"] = settings.brightness;
    doc["currentCard"] = settings.currentCard;
    doc["autoSwitch"] = settings.autoSwitch;
    doc["switchInterval"] = settings.switchInterval;
    doc["transitionType"] = settings.transitionType;
    
    // 添加元数据
    doc["version"] = EEPROM_VERSION;
    doc["timestamp"] = millis();
    
    // 序列化
    size_t len = serializeJson(doc, buffer, bufferSize);
    
    if (len >= bufferSize) {
        Serial.println(F("[DisplayConfig] Warning: JSON buffer overflow"));
        // 缓冲区不够，返回0表示错误
        buffer[0] = '\0';
        return 0;
    }
    
    return len;
}

void DisplayConfig::resetToDefault() {
    setDefaults();
    saveToEEPROM();
}

void DisplayConfig::clearEEPROM() {
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

void DisplayConfig::printConfig() {
    Serial.println(F("[DisplayConfig] Current Settings:"));
    Serial.printf("  Brightness: %d\n", settings.brightness);
    Serial.printf("  CurrentCard: %d\n", settings.currentCard);
    Serial.printf("  AutoSwitch: %s\n", settings.autoSwitch ? "true" : "false");
    Serial.printf("  SwitchInterval: %d seconds\n", settings.switchInterval);
    Serial.printf("  TransitionType: %d\n", settings.transitionType);
}

uint16_t DisplayConfig::getChecksum() {
    return calculateChecksum(settings);
}

// 私有方法实现

void DisplayConfig::loadFromEEPROM() {
    // 读取魔数
    uint32_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    
    if (magic != EEPROM_MAGIC) {
        Serial.println(F("[DisplayConfig] No valid config found, using defaults"));
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    // 读取版本
    uint16_t version;
    EEPROM.get(EEPROM_VERSION_ADDR, version);
    
    if (version != EEPROM_VERSION) {
        Serial.printf("[DisplayConfig] Version mismatch (%d vs %d), using defaults\n", 
                      version, EEPROM_VERSION);
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    // 读取配置数据
    DisplaySettings tempSettings;
    EEPROM.get(EEPROM_DISPLAY_SETTINGS_ADDR, tempSettings);
    
    // 验证校验和
    uint16_t storedChecksum;
    EEPROM.get(EEPROM_CHECKSUM_ADDR, storedChecksum);
    
    uint16_t calculatedChecksum = calculateChecksum(tempSettings);
    
    if (storedChecksum != calculatedChecksum) {
        Serial.println(F("[DisplayConfig] Checksum mismatch, using defaults"));
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    // 验证数据有效性
    if (!validateData(tempSettings)) {
        Serial.println(F("[DisplayConfig] Data validation failed, using defaults"));
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    settings = tempSettings;
    Serial.println(F("[DisplayConfig] Config loaded from EEPROM"));
}

void DisplayConfig::saveToEEPROM() {
    // 写入魔数
    uint32_t magic = EEPROM_MAGIC;
    EEPROM.put(EEPROM_MAGIC_ADDR, magic);
    
    // 写入版本
    uint16_t version = EEPROM_VERSION;
    EEPROM.put(EEPROM_VERSION_ADDR, version);
    
    // 写入配置
    EEPROM.put(EEPROM_DISPLAY_SETTINGS_ADDR, settings);
    
    // 计算并写入校验和
    uint16_t checksum = calculateChecksum(settings);
    EEPROM.put(EEPROM_CHECKSUM_ADDR, checksum);
    
    // 提交写入
    EEPROM.commit();
    
    Serial.println(F("[DisplayConfig] Config saved to EEPROM"));
}

uint16_t DisplayConfig::calculateChecksum(const DisplaySettings& data) {
    // 简单的校验和算法
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
    uint16_t checksum = 0;
    
    for (size_t i = 0; i < sizeof(DisplaySettings); i++) {
        checksum += ptr[i];
        checksum = (checksum << 1) | (checksum >> 15);  // 循环左移
    }
    
    return checksum;
}

bool DisplayConfig::validateData(const DisplaySettings& data) {
    // 检查亮度范围
    if (data.brightness > 255) return false;
    
    // 检查切换间隔
    if (data.switchInterval == 0 || data.switchInterval > 3600) return false;
    
    // 检查转场类型
    if (data.transitionType > 5) return false;
    
    return true;
}

void DisplayConfig::setDefaults() {
    settings = DisplaySettings();  // 使用构造函数默认值
    Serial.println(F("[DisplayConfig] Default settings applied"));
}
