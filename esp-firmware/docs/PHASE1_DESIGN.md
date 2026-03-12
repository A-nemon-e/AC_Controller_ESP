# Phase 1: 硬件抽象层重构 - 详细设计文档

> **阶段**: @plan - Phase 1  
> **日期**: 2026-03-12  
> **状态**: 规划中

---

## 1. 目标

将test_3733_scanner的显示功能重构为模块化架构，使其可以集成到ac_controller项目中。

**成功标准**:
- [ ] 单文件拆分至多个独立模块
- [ ] 消除全局命名空间污染
- [ ] 统一字体渲染系统（消除重复代码）
- [ ] 修复WiFi凭证硬编码
- [ ] 添加看门狗保护
- [ ] 通过编译和基础测试

---

## 2. 模块边界定义

### 2.1 模块划分

```
lib/
├── IS31FL3733/                 # 完全复用，无需修改
│   ├── IS31FL3733.h
│   └── IS31FL3733.cpp
│
├── LEDMatrix/                  # 新增：显示抽象层
│   ├── LEDMatrix.h             # Framebuffer类
│   ├── LEDMatrix.cpp
│   ├── LEDDriver.h             # 硬件驱动封装
│   └── LEDDriver.cpp
│
├── FontRenderer/               # 新增：字体渲染引擎
│   ├── FontRenderer.h          # 模板化字体渲染
│   ├── FontRenderer.cpp
│   ├── FontData.h              # 字体数据定义
│   └── fonts/                  # 字体数据文件
│       ├── Font_3x5.h
│       ├── Font_5x7.h
│       ├── Font_3x9.h
│       ├── Font_5x5.h
│       ├── Font_6x9.h
│       └── Icons.h
│
└── DisplayConfig/              # 新增：显示配置
    ├── DisplayConfig.h         # 配置数据结构
    └── DisplayConfig.cpp
```

### 2.2 每个模块的输入/输出/依赖

#### Module 1: LEDMatrix（Framebuffer抽象）

**输入**:
- 像素坐标 (x, y)
- 像素亮度值 (0-255)
- 渲染命令

**输出**:
- 42x11 Framebuffer状态
- 硬件刷新信号

**依赖**:
- IS31FL3733驱动库
- Wire库（I2C通信）

**接口定义**:
```cpp
class LEDMatrix {
public:
    // 初始化
    static bool begin();
    static void clear();
    
    // 像素操作
    static void setPixel(uint8_t x, uint8_t y, uint8_t brightness);
    static uint8_t getPixel(uint8_t x, uint8_t y);
    
    // 批量操作
    static void fill(uint8_t brightness);
    static void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness);
    
    // 渲染
    static void refresh();  // 将buffer刷新到硬件
    
    // 亮度控制
    static void setGlobalBrightness(uint8_t gcc);
    static uint8_t getGlobalBrightness();
    
private:
    static uint8_t buffer[SCREEN_ROWS][SCREEN_COLS];
    static uint8_t globalBrightness;
    static IS31FL3733Driver* drivers[NUM_CHIPS];
};
```

**边界约束**:
- x范围: 0-41
- y范围: 0-10
- brightness范围: 0-255
- 线程安全：单线程访问（ESP8266）

---

#### Module 2: FontRenderer（字体渲染引擎）

**输入**:
- 字符/字符串
- 字体类型枚举
- 起始坐标 (x, y)
- 亮度值

**输出**:
- 在LEDMatrix上渲染的字符
- 字符串宽度计算

**依赖**:
- LEDMatrix
- 字体数据（PROGMEM存储）

**接口定义**:
```cpp
enum class FontType {
    FONT_3x5,
    FONT_5x7,
    FONT_3x9,
    FONT_5x5,
    FONT_6x9
};

class FontRenderer {
public:
    // 渲染单个字符
    static void drawChar(char c, uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    // 渲染字符串
    static void drawString(const char* str, uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    
    // 计算尺寸
    static uint8_t getCharWidth(FontType font);
    static uint8_t getCharHeight(FontType font);
    static uint16_t getStringWidth(const char* str, FontType font);
    
    // 计算居中位置
    static uint8_t centerX(const char* str, FontType font);
    
    // 绘制特殊符号
    static void drawColon(uint8_t x, uint8_t y, FontType font, uint8_t brightness = 255);
    static void drawIcon(const uint8_t* iconData, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t brightness = 255);
    
private:
    // 获取字体数据指针
    static const uint8_t* getFontData(char c, FontType font);
    static uint8_t getFontCharIndex(char c);
};
```

**边界约束**:
- 支持字符：0-9, A-Z, a-z, 冒号, 空格
- 超出屏幕自动裁剪
- 字符串截断处理

**消除重复的策略**:
```cpp
// 使用模板统一所有字体渲染
template<FontType FONT>
static void drawCharT(char c, uint8_t x, uint8_t y, uint8_t brightness) {
    constexpr uint8_t width = FontTraits<FONT>::width;
    constexpr uint8_t height = FontTraits<FONT>::height;
    constexpr const uint8_t (*fontData)[height] = FontTraits<FONT>::data;
    
    int idx = getFontCharIndex(c);
    if (idx < 0) return;
    
    for (uint8_t row = 0; row < height; row++) {
        uint8_t rowData = pgm_read_byte(&fontData[idx][row]);
        for (uint8_t col = 0; col < width; col++) {
            if (rowData & (1 << (width - 1 - col))) {
                LEDMatrix::setPixel(x + col, y + row, brightness);
            }
        }
    }
}
```

---

#### Module 3: LEDDriver（硬件驱动封装）

**输入**:
- IS31FL3733初始化命令
- PWM数据
- 配置参数

**输出**:
- I2C信号到硬件
- 芯片状态

**依赖**:
- IS31FL3733库
- Wire库

**接口定义**:
```cpp
class LEDDriver {
public:
    // 初始化所有6个芯片
    static bool init();
    
    // 发送PWM数据到指定芯片
    static void sendPWM(uint8_t chipIndex, const uint8_t* pwmData, uint8_t length);
    
    // 设置全局亮度
    static void setGlobalBrightness(uint8_t gcc);
    
    // 开关矩阵
    static void enable();
    static void disable();
    
    // 扫描I2C总线（调试）
    static void scanI2C();
    
private:
    static IS31FL3733Driver* drivers[NUM_CHIPS];
    static bool initialized;
    
    // I2C读写函数
    static uint8_t i2cWriteReg(uint8_t addr, uint8_t reg, const uint8_t* buf, uint8_t cnt);
    static uint8_t i2cReadReg(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t cnt);
};
```

**边界约束**:
- I2C地址: 0x50-0x55
- I2C速度: 400kHz
- 初始化顺序: chip5 -> chip0（同步需求）

---

#### Module 4: DisplayConfig（显示配置管理）

**输入**:
- EEPROM读取
- MQTT配置更新
- 用户按键操作

**输出**:
- 当前配置状态
- 配置持久化

**依赖**:
- EEPROM库
- ArduinoJson

**接口定义**:
```cpp
struct DisplaySettings {
    uint8_t brightness;          // 全局亮度 0-255
    uint8_t currentCard;         // 当前卡片索引
    bool autoSwitch;             // 是否自动切换卡片
    uint16_t switchInterval;     // 切换间隔（秒）
    uint8_t transitionType;      // 转场效果类型
};

class DisplayConfig {
public:
    // 初始化（从EEPROM加载）
    static bool init();
    
    // 获取配置
    static DisplaySettings getSettings();
    static uint8_t getBrightness();
    
    // 设置配置
    static void setSettings(const DisplaySettings& settings);
    static void setBrightness(uint8_t brightness);
    static void setCurrentCard(uint8_t cardIndex);
    
    // 持久化
    static void save();
    
    // 从JSON更新（MQTT）
    static bool updateFromJSON(const char* json);
    
    // 重置为默认
    static void resetToDefault();
    
private:
    static DisplaySettings settings;
    static bool dirty;  // 是否需要保存
    
    static void loadFromEEPROM();
    static void saveToEEPROM();
};
```

**EEPROM布局**:
```
Address 0x00-0x10:  魔数 + 版本 + 校验和
Address 0x10-0x30:  DisplaySettings结构
Address 0x30-0x50:  卡片配置（预留）
Address 0x50-0x100: 用户数据（预留）
```

---

## 3. 依赖关系图

```
┌─────────────────────────────────────────────────────────────┐
│                     Module Dependencies                      │
└─────────────────────────────────────────────────────────────┘

                    ┌──────────────┐
                    │  Application │
                    │ ac_controller│
                    └──────┬───────┘
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
    │ FontRenderer│  │DisplayConfig│  │  (其他)     │
    └──────┬──────┘  └──────┬──────┘  └─────────────┘
           │                │
           ▼                ▼
    ┌─────────────┐  ┌─────────────┐
    │  LEDMatrix  │  │    EEPROM   │
    └──────┬──────┘  └─────────────┘
           │
           ▼
    ┌─────────────┐
    │  LEDDriver  │
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │ IS31FL3733  │
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Wire(I2C) │
    └─────────────┘
```

---

## 4. 关键设计决策

### 决策1: 单例模式 vs 静态类

**选择**: 使用静态类（所有方法为static）

**理由**:
- ESP8266是单线程环境，无需多实例
- 静态类比单例更简洁，无实例化开销
- 符合Arduino库习惯

**例外**: 如果后续需要支持多屏，可改为实例化模式

---

### 决策2: 错误处理策略

**策略**: 返回值 + 内部状态

```cpp
class LEDMatrix {
public:
    // 返回bool表示是否成功
    static bool setPixel(uint8_t x, uint8_t y, uint8_t brightness) {
        if (x >= SCREEN_COLS || y >= SCREEN_ROWS) {
            lastError = Error::OUT_OF_BOUNDS;
            return false;
        }
        buffer[y][x] = brightness;
        return true;
    }
    
    // 获取最后错误
    static Error getLastError() { return lastError; }
    static void clearError() { lastError = Error::NONE; }
    
private:
    static Error lastError;
};
```

---

### 决策3: 字体数据存储

**选择**: PROGMEM（Flash存储）

**理由**:
- ESP8266 RAM有限（80KB）
- 字体数据约4KB，占用RAM太浪费
- 使用pgm_read_byte()读取

**实现**:
```cpp
// FontData.h
static const uint8_t FONT_3X5_DATA[36][5] PROGMEM = {
    // ... 字体数据
};
```

---

### 决策4: WiFi凭证管理

**当前问题**:
```cpp
// test_3733_scanner.ino - 硬编码
const char *WIFI_SSID = "TP-LINK_AFC5F2";
const char *WIFI_PASS = "2002051377";
```

**解决方案**:
使用ac_controller的现有方案 - WiFiManager类
- 首次启动进入AP模式配置
- 凭证存储在EEPROM
- 支持SmartConfig

**无需新建模块，复用ac_controller/wifi_manager.h**

---

### 决策5: 看门狗保护

**实现位置**: LEDMatrix::refresh() 和长时间动画循环

```cpp
void LEDMatrix::refresh() {
    // 喂狗，防止长时间刷新触发复位
    ESP.wdtFeed();
    
    // 刷新所有芯片
    for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
        drivers[chip]->SetPWM(pwmBuffers[chip]);
        ESP.wdtFeed();  // 每芯片喂狗一次
    }
}
```

---

## 5. 接口契约

### 5.1 LEDMatrix → LEDDriver

**调用方式**: LEDMatrix直接调用LEDDriver
**时序**: 立即执行
**错误处理**: LEDDriver返回bool，LEDMatrix记录错误

### 5.2 FontRenderer → LEDMatrix

**调用方式**: FontRenderer调用LEDMatrix::setPixel()
**坐标约定**: 
- 原点(0,0)在左上角
- X向右增加
- Y向下增加
**裁剪**: LEDMatrix自动处理越界

### 5.3 DisplayConfig → EEPROM

**调用方式**: DisplayConfig使用EEPROM库
**写入策略**: 延迟写入（标记dirty，定期保存）
**磨损均衡**: 使用EEPROM.put()而非write()

---

## 6. 测试策略

### 6.1 单元测试（在主机上模拟）

**方法**: 使用ArduinoFake或自定义mock

```cpp
// test_led_matrix.cpp
void test_setPixel() {
    LEDMatrix::begin();
    LEDMatrix::clear();
    
    LEDMatrix::setPixel(0, 0, 255);
    assert(LEDMatrix::getPixel(0, 0) == 255);
    
    // 测试越界
    assert(!LEDMatrix::setPixel(100, 100, 255));
}
```

### 6.2 集成测试（在ESP8266上）

**测试用例**:
1. **初始化测试**: 所有芯片是否正确初始化
2. **像素测试**: 逐个点亮所有462个LED
3. **字体测试**: 显示所有字符
4. **亮度测试**: 不同亮度级别
5. **长时间稳定性**: 运行24小时

### 6.3 硬件在环测试

**工具**: 串口监视器 + 逻辑分析仪（可选）

**检查点**:
- I2C通信是否正常
- 芯片地址是否正确（0x50-0x55）
- PWM数据是否一致

---

## 7. 实施顺序

### Week 1.1: 基础框架
1. [ ] 创建lib目录结构
2. [ ] 复制IS31FL3733驱动
3. [ ] 实现LEDDriver类
4. [ ] 实现LEDMatrex类（基础功能）

### Week 1.2: 字体系统
1. [ ] 提取字体数据到PROGMEM
2. [ ] 实现FontRenderer（模板化）
3. [ ] 测试所有字体渲染
4. [ ] 修复WiFi凭证问题

### Week 2.1: 配置管理
1. [ ] 实现DisplayConfig
2. [ ] EEPROM读写测试
3. [ ] MQTT配置更新测试

### Week 2.2: 集成与测试
1. [ ] 集成到ac_controller
2. [ ] 添加看门狗保护
3. [ ] 完整系统测试
4. [ ] 性能优化

---

## 8. 风险与缓解

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| I2C通信不稳定 | 中 | 高 | 添加重试机制、错误恢复 |
| RAM不足 | 低 | 高 | 使用PROGMEM、优化buffer |
| 字体渲染性能差 | 中 | 中 | 批量渲染、PROGMEM读取优化 |
| EEPROM磨损 | 低 | 中 | 延迟写入、减少保存频率 |

---

## 9. 代码规范

### 命名规范
- 类名: PascalCase (`LEDMatrex`)
- 方法名: camelCase (`setPixel`)
- 常量: UPPER_SNAKE_CASE (`SCREEN_COLS`)
- 私有成员: 下划线前缀 (`_buffer`)

### 文件规范
- 头文件: `.h`（Arduino风格）
- 实现文件: `.cpp`
- 每个类独立文件

### 注释规范
```cpp
/**
 * @brief 设置指定坐标的像素亮度
 * @param x X坐标 (0-41)
 * @param y Y坐标 (0-10)
 * @param brightness 亮度值 (0-255)
 * @return true成功，false越界
 */
static bool setPixel(uint8_t x, uint8_t y, uint8_t brightness);
```

---

**下一步**: 创建Phase 1的实施计划（每个Task的详细步骤）
