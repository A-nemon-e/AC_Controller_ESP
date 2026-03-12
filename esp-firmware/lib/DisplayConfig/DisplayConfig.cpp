/**
 * DisplayConfig - 显示配置管理实现
 */

#include "DisplayConfig.h"

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

void DisplayConfig::setTransitionType(TransitionType type) {
    settings.transitionType = static_cast<uint8_t>(type);
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

bool DisplayConfig::updateFromJSON(const char* json) {
    // 简易JSON解析（不使用ArduinoJson库，减少依赖）
    // 格式示例: {"brightness":128,"autoSwitch":true,"switchInterval":30}
    
    char* ptr = strstr(json, "\"brightness\"");
    if (ptr) {
        ptr = strchr(ptr, ':');
        if (ptr) {
            int val = atoi(ptr + 1);
            if (val >= 0 && val <= 255) {
                settings.brightness = (uint8_t)val;
            }
        }
    }
    
    ptr = strstr(json, "\"autoSwitch\"");
    if (ptr) {
        ptr = strchr(ptr, ':');
        if (ptr) {
            settings.autoSwitch = (strstr(ptr, "true") != nullptr);
        }
    }
    
    ptr = strstr(json, "\"switchInterval\"");
    if (ptr) {
        ptr = strchr(ptr, ':');
        if (ptr) {
            int val = atoi(ptr + 1);
            if (val > 0 && val < 3600) {
                settings.switchInterval = (uint16_t)val;
            }
        }
    }
    
    ptr = strstr(json, "\"transitionType\"");
    if (ptr) {
        ptr = strchr(ptr, ':');
        if (ptr) {
            int val = atoi(ptr + 1);
            if (val >= 0 && val <= 5) {
                settings.transitionType = (uint8_t)val;
            }
        }
    }
    
    markDirty();
    return true;
}

size_t DisplayConfig::toJSON(char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize,
        "{\"brightness\":%d,\"currentCard\":%d,\"autoSwitch\":%s,\"switchInterval\":%d,\"transitionType\":%d}",
        settings.brightness,
        settings.currentCard,
        settings.autoSwitch ? "true" : "false",
        settings.switchInterval,
        settings.transitionType
    );
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
    Serial.println("[DisplayConfig] Current Settings:");
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
        Serial.println("[DisplayConfig] No valid config found, using defaults");
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    // 读取版本
    uint16_t version;
    EEPROM.get(EEPROM_VERSION_ADDR, version);
    
    if (version != EEPROM_VERSION) {
        Serial.printf("[DisplayConfig] Version mismatch (%d vs %d), using defaults\n", version, EEPROM_VERSION);
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
        Serial.println("[DisplayConfig] Checksum mismatch, using defaults");
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    // 验证数据有效性
    if (!validateData(tempSettings)) {
        Serial.println("[DisplayConfig] Data validation failed, using defaults");
        setDefaults();
        saveToEEPROM();
        return;
    }
    
    settings = tempSettings;
    Serial.println("[DisplayConfig] Config loaded from EEPROM");
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
    
    Serial.println("[DisplayConfig] Config saved to EEPROM");
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
    Serial.println("[DisplayConfig] Default settings applied");
}
