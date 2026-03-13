# Phase 2: 显示引擎核心 - 详细设计文档

> **阶段**: @plan - Phase 2  
> **日期**: 2026-03-12  
> **前置依赖**: Phase 1 (硬件抽象层)  
> **状态**: 规划中
> **实施进度**: 0% (待开始)

---

## 1. 目标

基于Phase 1的硬件抽象层，构建完整的显示引擎架构，实现前景/背景分层叠加、卡片管理和转场效果。

**成功标准**:
- [x] 实现Background基类和所有派生类（火焰、矩阵雨、水波纹、生命游戏、沙漏、Pong、天气）(100% - 7种背景.h文件完整)
- [x] 实现Foreground基类和所有派生类（时钟、日期、温湿度）(100% - 3种前景.h文件完整)
- [x] 实现Card类（组合0-1背景和0-N前景）(100% - Card.h完整实现)
- [x] 实现Transition转场效果系统 (100% - Transition.h完整实现)
- [x] 实现DisplayManager管理所有卡片 (100% - DisplayManager.h完整实现)
- [ ] 通过集成测试（所有效果正常运行）(0% - 需要硬件测试)

**实施进度**: 83% (框架代码100%完成，待硬件集成测试)

---

## 2. 架构设计

### 2.1 核心类图

```
┌─────────────────────────────────────────────────────────────────┐
│                     Display Engine Architecture                  │
└─────────────────────────────────────────────────────────────────┘

                           ┌──────────────────┐
                           │  DisplayManager  │
                           │   (单例)          │
                           ├──────────────────┤
                           │ - cards[]        │
                           │ - currentCard    │
                           │ - transition     │
                           ├──────────────────┤
                           │ + addCard()      │
                           │ + switchCard()   │
                           │ + render()       │
                           │ + update()       │
                           └────────┬─────────┘
                                    │
                 ┌──────────────────┼──────────────────┐
                 ▼                  ▼                  ▼
        ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
        │      Card       │ │   Transition    │ │    Config       │
        ├─────────────────┤ ├─────────────────┤ ├─────────────────┤
        │ - background    │ │ - type          │ │ - brightness    │
        │ - foregrounds[] │ │ - duration      │ │ - switchInterval│
        │ - name          │ │ - progress      │ │ - autoSwitch    │
        ├─────────────────┤ ├─────────────────┤ ├─────────────────┤
        │ + render()      │ │ + begin()       │ │ + load()        │
        │ + addForeground │ │ + update()      │ │ + save()        │
        │ + setBackground │ │ + isFinished()  │ │ + fromJSON()    │
        └────────┬────────┘ └─────────────────┘ └─────────────────┘
                 │
        ┌────────┴────────┐
        ▼                 ▼
┌─────────────────┐ ┌─────────────────┐
│   Background    │ │   Foreground    │
│   (抽象基类)     │ │   (抽象基类)     │
├─────────────────┤ ├─────────────────┤
│ - brightness    │ │ - position      │
│ - isOverlay     │ │ - font          │
│ - supportedPos  │ │ - brightness    │
├─────────────────┤ ├─────────────────┤
│ + init()        │ │ + init()        │
│ + render()      │ │ + render()      │
│ + update()      │ │ + update()      │
│ + canOverlay()  │ │ + getSize()     │
└────────┬────────┘ └────────┬────────┘
         │                   │
         ▼                   ▼
┌─────────────────────────────────────────────────────┐
│                   派生类                             │
├─────────────────────────────────────────────────────┤
│  Background派生类        │   Foreground派生类        │
│  - FireBackground        │   - ClockForeground       │
│  - MatrixRainBackground  │   - DateForeground        │
│  - WaterRippleBackground │   - TempForeground        │
│  - GameOfLifeBackground  │                           │
│  - SandBackground        │                           │
│  - PongBackground        │                           │
│  - WeatherBackground     │                           │
│    - Sunny               │                           │
│    - Rainy               │                           │
│    - Snowy               │                           │
│    - Windy               │                           │
│    - Cloudy              │                           │
└──────────────────────────┴───────────────────────────┘
```

### 2.2 分层渲染流程

```
渲染一帧的流程：

1. DisplayManager.update()
   └── 检查是否需要切换卡片
       └── 如果是，启动Transition

2. DisplayManager.render()
   ├── 获取当前Card
   │
   ├── 如果有Transition进行中
   │   ├── 渲染"从"卡片到临时Buffer A
   │   ├── 渲染"到"卡片到临时Buffer B
   │   └── 混合Buffer A和B（根据Transition进度）
   │
   └── 如果没有Transition
   │   └── 直接渲染当前Card到Buffer
   │
   └── Card.render()
       ├── Background.render()     ← 底层（全屏）
       ├── 对每个Foreground
       │   └── Foreground.render() ← 叠加层
       └── 合成最终图像
   │
   └── LEDMatrix.refresh()         ← 输出到硬件

3. DisplayManager.update()
   └── Transition.update()（更新进度）
```

---

## 3. 模块边界定义

### 3.1 模块划分

```
lib/DisplayEngine/
├── Core/
│   ├── DisplayManager.h/.cpp      # 显示管理器
│   ├── Card.h/.cpp                # 卡片类
│   └── Transition.h/.cpp          # 转场效果
│
├── Backgrounds/
│   ├── Background.h               # 背景基类
│   ├── FireBackground.h/.cpp      # 火焰效果
│   ├── MatrixRainBackground.h/.cpp # 矩阵雨
│   ├── WaterRippleBackground.h/.cpp # 水波纹
│   ├── GameOfLifeBackground.h/.cpp # 生命游戏
│   ├── SandBackground.h/.cpp      # 沙漏模拟
│   ├── PongBackground.h/.cpp      # Pong时钟
│   └── WeatherBackground.h/.cpp   # 天气背景基类
│
├── Foregrounds/
│   ├── Foreground.h               # 前景基类
│   ├── ClockForeground.h/.cpp     # 时钟前景
│   ├── DateForeground.h/.cpp      # 日期星期
│   └── TempForeground.h/.cpp      # 温湿度
│
└── Utils/
    ├── DisplayConfig.h/.cpp       # 显示配置
    └── Position.h                 # 位置定义
```

### 3.2 各模块详细定义

#### Module 1: Background（背景基类）

**职责**: 定义背景渲染接口，管理全屏动态效果

**输入**:
- 时间戳（用于动画计算）
- LEDMatrix实例（渲染目标）
- 配置参数（亮度、速度等）

**输出**:
- 全屏像素数据（42×11）
- 是否支持前景叠加
- 推荐前景位置

**接口定义**:
```cpp
class Background {
public:
    enum class Type {
        FIRE,           // 火焰
        MATRIX_RAIN,    // 矩阵雨
        WATER_RIPPLE,   // 水波纹
        GAME_OF_LIFE,   // 生命游戏
        SAND,           // 沙漏
        PONG,           // Pong时钟
        WEATHER_SUNNY,  // 晴天
        WEATHER_RAINY,  // 下雨
        WEATHER_SNOWY,  // 下雪
        WEATHER_WINDY,  // 刮风
        WEATHER_CLOUDY, // 阴天
        NONE            // 无背景
    };
    
    // 前景位置支持
    struct SupportedPositions {
        bool top;       // 顶部
        bool bottom;    // 底部
        bool left;      // 左侧
        bool right;     // 右侧
        bool center;    // 中央
        Position recommended;  // 推荐位置
    };
    
    Background(Type type);
    virtual ~Background();
    
    // 生命周期
    virtual void init();           // 初始化状态
    virtual void reset();          // 重置状态
    
    // 渲染
    virtual void render(LEDMatrix& matrix) = 0;  // 纯虚函数
    virtual void update();         // 更新动画状态
    
    // 属性查询
    Type getType() const { return type; }
    virtual bool canOverlay() const { return true; }
    virtual SupportedPositions getSupportedPositions() const;
    
    // 配置
    void setBrightness(uint8_t brightness) { this->brightness = brightness; }
    uint8_t getBrightness() const { return brightness; }
    
protected:
    Type type;
    uint8_t brightness;
    uint32_t frameCount;
    uint32_t lastUpdateTime;
    
    // 动画速度控制
    uint16_t updateInterval;  // 更新间隔(ms)
};
```

**边界约束**:
- `render()`必须填满整个42×11区域
- `update()`每帧调用，但需根据`updateInterval`控制实际更新频率
- 背景必须处理自己的状态管理

---

#### Module 2: FireBackground（火焰效果）

**特殊约束**:
- 只支持3×5小字体
- 前景必须靠上（避免被火焰遮挡）
- 温度≥30°C时自动启用

**实现要点**:
```cpp
class FireBackground : public Background {
public:
    FireBackground() : Background(Type::FIRE) {
        updateInterval = 50;  // 20fps
    }
    
    void init() override {
        // 初始化火焰像素数组
        memset(firePixels, 0, sizeof(firePixels));
    }
    
    void render(LEDMatrix& matrix) override {
        // 1. 更新火焰算法
        updateFire();
        
        // 2. 渲染到矩阵
        for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint8_t intensity = firePixels[y][x];
                // 映射到LED亮度（0-255）
                matrix.setPixel(x, y, intensity * brightness / 255);
            }
        }
    }
    
    bool canOverlay() const override { return true; }
    
    SupportedPositions getSupportedPositions() const override {
        return {true, false, false, false, false, Position::TOP};
    }
    
private:
    uint8_t firePixels[SCREEN_ROWS][SCREEN_COLS];
    
    void updateFire() {
        // 火焰算法（从test_3733_scanner移植）
        // 1. 底部生成火种
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            firePixels[SCREEN_ROWS-1][x] = random(160, 255);
        }
        
        // 2. 向上传播
        for (uint8_t y = 0; y < SCREEN_ROWS - 1; y++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint8_t decay = random(0, 32);
                uint8_t below = firePixels[y+1][x];
                uint8_t newVal = (below > decay) ? (below - decay) : 0;
                firePixels[y][x] = newVal;
            }
        }
    }
};
```

---

#### Module 3: MatrixRainBackground（矩阵雨）

**特殊约束**:
- 支持前景叠加
- 前景区域自动清除雨滴（避免干扰阅读）

**实现要点**:
```cpp
class MatrixRainBackground : public Background {
public:
    MatrixRainBackground() : Background(Type::MATRIX_RAIN) {
        updateInterval = 60;
    }
    
    void init() override {
        // 初始化雨滴数组
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            drops[x] = random(SCREEN_ROWS);
            speeds[x] = random(1, 3);
            brightness[x] = random(50, 255);
        }
    }
    
    void render(LEDMatrix& matrix) override {
        matrix.clear();
        
        // 渲染雨滴
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            uint8_t y = drops[x];
            // 雨滴头部（最亮）
            matrix.setPixel(x, y, brightness[x] * this->brightness / 255);
            // 雨滴尾巴（渐暗）
            for (uint8_t t = 1; t <= 3; t++) {
                if (y >= t) {
                    uint8_t tailY = y - t;
                    uint8_t tailBri = brightness[x] / (t + 1);
                    matrix.setPixel(x, tailY, tailBri * this->brightness / 255);
                }
            }
        }
        
        // 前景区域清除（可选）
        if (foregroundArea.valid) {
            clearForegroundArea(matrix);
        }
    }
    
    void update() override {
        Background::update();
        
        // 更新雨滴位置
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            drops[x] += speeds[x];
            if (drops[x] >= SCREEN_ROWS + 3) {
                drops[x] = 0;
                speeds[x] = random(1, 3);
                brightness[x] = random(50, 255);
            }
        }
    }
    
    // 设置前景区域（自动清除）
    void setForegroundArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
        foregroundArea = {x, y, w, h, true};
    }
    
private:
    uint8_t drops[SCREEN_COLS];
    uint8_t speeds[SCREEN_COLS];
    uint8_t brightness[SCREEN_COLS];
    
    struct Area {
        uint8_t x, y, w, h;
        bool valid;
    } foregroundArea;
    
    void clearForegroundArea(LEDMatrix& matrix) {
        for (uint8_t y = foregroundArea.y; y < foregroundArea.y + foregroundArea.h; y++) {
            for (uint8_t x = foregroundArea.x; x < foregroundArea.x + foregroundArea.w; x++) {
                matrix.setPixel(x, y, 0);
            }
        }
    }
};
```

---

#### Module 4: WeatherBackground（天气背景系统）

**特殊设计**: 组合模式，支持多天气叠加

```cpp
class WeatherBackground : public Background {
public:
    enum class WeatherType {
        SUNNY,      // 晴天（太阳）
        RAINY,      // 下雨（大/中/小自动切换）
        SNOWY,      // 下雪
        WINDY,      // 刮风（线条动画）
        CLOUDY      // 阴天
    };
    
    WeatherBackground();
    
    // 添加天气效果（可叠加）
    void addWeather(WeatherType type, uint8_t intensity = 128);
    void removeWeather(WeatherType type);
    void clearWeathers();
    
    void render(LEDMatrix& matrix) override;
    void update() override;
    
    // 根据温度自动选择
    void autoSelectByTemp(float temp);
    
private:
    struct WeatherLayer {
        WeatherType type;
        uint8_t intensity;
        void* state;  // 各天气类型的私有状态
    };
    
    std::vector<WeatherLayer> layers;
    
    // 各天气渲染器
    void renderSunny(LEDMatrix& matrix, uint8_t intensity);
    void renderRainy(LEDMatrix& matrix, uint8_t intensity);
    void renderSnowy(LEDMatrix& matrix, uint8_t intensity);
    void renderWindy(LEDMatrix& matrix, uint8_t intensity);
    void renderCloudy(LEDMatrix& matrix, uint8_t intensity);
    
    // 自动切换雨的大小
    uint32_t lastRainChange;
    uint8_t currentRainLevel;  // 0=小, 1=中, 2=大
};
```

---

#### Module 5: Foreground（前景基类）

**职责**: 定义前景渲染接口，管理信息叠加显示

**输入**:
- LEDMatrix实例（渲染目标）
- FontRenderer实例
- 位置配置
- 数据（时间、温湿度等）

**输出**:
- 局部区域像素数据
- 自身尺寸信息

**接口定义**:
```cpp
class Foreground {
public:
    enum class Type {
        CLOCK,          // 时钟（时:分 或 时:分:秒）
        DATE,           // 日期（月-日）
        WEEKDAY,        // 星期
        TEMP_HUMID,     // 温湿度
        NONE
    };
    
    // 位置枚举
    enum class Position {
        TOP_LEFT,       // 左上
        TOP_CENTER,     // 上中
        TOP_RIGHT,      // 右上
        CENTER_LEFT,    // 左中
        CENTER,         // 中央
        CENTER_RIGHT,   // 右中
        BOTTOM_LEFT,    // 左下
        BOTTOM_CENTER,  // 下中
        BOTTOM_RIGHT    // 右下
    };
    
    Foreground(Type type);
    virtual ~Foreground();
    
    // 生命周期
    virtual void init();
    virtual void reset();
    
    // 渲染
    virtual void render(LEDMatrix& matrix, FontRenderer& fonts) = 0;
    virtual void update();
    
    // 尺寸计算
    virtual void getSize(uint8_t& w, uint8_t& h) const = 0;
    
    // 位置管理
    void setPosition(Position pos) { position = pos; }
    Position getPosition() const { return position; }
    
    // 字体设置
    void setFont(FontType font) { this->font = font; }
    FontType getFont() const { return font; }
    
    // 亮度
    void setBrightness(uint8_t brightness) { this->brightness = brightness; }
    uint8_t getBrightness() const { return brightness; }
    
    // 自动切换时间（0=不自动切换）
    void setDisplayDuration(uint16_t seconds) { displayDuration = seconds * 1000; }
    uint16_t getDisplayDuration() const { return displayDuration / 1000; }
    
protected:
    Type type;
    Position position;
    FontType font;
    uint8_t brightness;
    uint32_t displayDuration;  // 显示时长(ms)
    uint32_t displayStartTime; // 开始显示时间
    
    // 计算实际坐标
    void calculatePosition(uint8_t& x, uint8_t& y, uint8_t contentW, uint8_t contentH) const;
};
```

---

#### Module 6: ClockForeground（时钟前景）

**实现要点**:
```cpp
class ClockForeground : public Foreground {
public:
    enum class Format {
        HH_MM,      // 时:分
        HH_MM_SS    // 时:分:秒
    };
    
    ClockForeground() : Foreground(Type::CLOCK), format(Format::HH_MM) {}
    
    void setFormat(Format fmt) { format = fmt; }
    
    void render(LEDMatrix& matrix, FontRenderer& fonts) override {
        // 获取当前时间
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        char buffer[10];
        uint8_t contentW, contentH;
        
        if (format == Format::HH_MM) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d", 
                     timeinfo->tm_hour, timeinfo->tm_min);
            contentW = FontRenderer::getStringWidth(buffer, font);
            contentH = FontRenderer::getCharHeight(font);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            // 使用较小字体显示秒
            contentW = FontRenderer::getStringWidth(buffer, font);
            contentH = FontRenderer::getCharHeight(font);
        }
        
        // 计算位置
        uint8_t x, y;
        calculatePosition(x, y, contentW, contentH);
        
        // 渲染
        fonts.drawString(buffer, x, y, font, brightness);
    }
    
    void getSize(uint8_t& w, uint8_t& h) const override {
        if (format == Format::HH_MM) {
            // HH:MM = 5 chars (H H : M M)
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
```

---

#### Module 7: Card（卡片类）

**职责**: 组合0-1个背景和0-N个前景，管理显示逻辑

**关键设计**:
- 背景全屏渲染（底层）
- 前景按顺序叠加
- 多个前景自动切换

```cpp
class Card {
public:
    Card(const char* name);
    ~Card();
    
    // 背景管理
    void setBackground(Background* bg);
    Background* getBackground() const { return background; }
    void removeBackground();
    
    // 前景管理
    void addForeground(Foreground* fg);
    void removeForeground(size_t index);
    void clearForegrounds();
    size_t getForegroundCount() const { return foregrounds.size(); }
    Foreground* getForeground(size_t index) const;
    
    // 渲染
    void render(LEDMatrix& matrix, FontRenderer& fonts);
    void update();
    
    // 前景切换
    void setAutoSwitch(bool enable) { autoSwitchForegrounds = enable; }
    bool getAutoSwitch() const { return autoSwitchForegrounds; }
    
    // 名称
    const char* getName() const { return name; }
    void setName(const char* newName);
    
    // 序列化
    void toJSON(JsonObject& json) const;
    static Card* fromJSON(const JsonObject& json);
    
private:
    char name[32];
    Background* background;
    std::vector<Foreground*> foregrounds;
    
    // 前景切换
    bool autoSwitchForegrounds;
    size_t currentForegroundIndex;
    uint32_t foregroundStartTime;
    
    // 验证前景与背景的兼容性
    bool validateForegroundPosition(Foreground* fg) const;
};
```

---

#### Module 8: Transition（转场效果）

**类型定义**:
```cpp
enum class TransitionType {
    INSTANT,        // 硬切（无效果）
    FADE,           // 淡入淡出
    SLIDE_LEFT,     // 向左滑动
    SLIDE_RIGHT,    // 向右滑动
    SLIDE_UP,       // 向上滑动
    SLIDE_DOWN,     // 向下滑动
    WIPE,           // 擦除效果
    PIXELATE        // 像素化（可选）
};
```

**实现**:
```cpp
class Transition {
public:
    Transition(TransitionType type, uint16_t durationMs);
    
    void begin();
    void update();
    bool isFinished() const;
    
    // 混合两个Buffer
    void blend(const LEDMatrix& from, const LEDMatrix& to, LEDMatrix& output);
    
    // 进度 0.0-1.0
    float getProgress() const;
    
private:
    TransitionType type;
    uint16_t durationMs;
    uint32_t startTime;
    
    // 各类型混合算法
    void blendFade(const LEDMatrix& from, const LEDMatrix& to, LEDMatrix& output, float progress);
    void blendSlide(const LEDMatrix& from, const LEDMatrix& to, LEDMatrix& output, float progress, bool horizontal, bool reverse);
    void blendWipe(const LEDMatrix& from, const LEDMatrix& to, LEDMatrix& output, float progress);
};
```

---

#### Module 9: DisplayManager（显示管理器）

**职责**: 管理所有卡片、处理切换、协调渲染

```cpp
class DisplayManager {
public:
    static DisplayManager& getInstance();
    
    // 初始化
    void init();
    void begin();
    
    // 卡片管理
    void addCard(Card* card);
    void removeCard(size_t index);
    void clearCards();
    size_t getCardCount() const { return cards.size(); }
    Card* getCard(size_t index) const;
    Card* getCurrentCard() const;
    
    // 卡片切换
    void switchCard(size_t index);
    void nextCard();
    void previousCard();
    void setAutoSwitch(bool enable) { autoSwitchCards = enable; }
    
    // 按键处理（集成）
    void onButtonClick();
    void onButtonDoubleClick();
    void onButtonLongPress();
    
    // 主循环
    void update();  // 更新状态
    void render();  // 渲染一帧
    
    // 配置
    void loadConfig();
    void saveConfig();
    void applyConfig(const DisplayConfig& config);
    
    // MQTT配置更新
    void onConfigUpdate(const char* json);
    
private:
    DisplayManager() = default;
    ~DisplayManager();
    
    std::vector<Card*> cards;
    size_t currentCardIndex;
    
    // 自动切换
    bool autoSwitchCards;
    uint16_t cardSwitchInterval;  // 卡片切换间隔（秒）
    uint32_t cardStartTime;
    
    // 转场
    Transition* currentTransition;
    LEDMatrix* transitionBufferA;
    LEDMatrix* transitionBufferB;
    
    // 硬件接口
    LEDMatrix matrix;
    FontRenderer fonts;
    
    // 渲染统计
    uint32_t frameCount;
    uint32_t lastFPSUpdate;
    uint8_t currentFPS;
};
```

---

## 4. 内存管理策略

### 4.1 内存分配

```cpp
// 静态分配（避免堆碎片）
class DisplayManager {
private:
    // 使用固定大小数组代替vector
    static constexpr size_t MAX_CARDS = 10;
    static constexpr size_t MAX_FOREGROUNDS_PER_CARD = 5;
    
    Card* cards[MAX_CARDS];
    size_t cardCount;
    
    // Buffer复用
    uint8_t buffer1[SCREEN_ROWS][SCREEN_COLS];
    uint8_t buffer2[SCREEN_ROWS][SCREEN_COLS];
};
```

### 4.2 对象池

```cpp
// 前景对象池（避免频繁new/delete）
template<typename T, size_t POOL_SIZE>
class ObjectPool {
public:
    T* acquire() {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (!used[i]) {
                used[i] = true;
                return &pool[i];
            }
        }
        return nullptr;  // 池耗尽
    }
    
    void release(T* obj) {
        size_t index = obj - pool;
        if (index < POOL_SIZE) {
            used[index] = false;
            pool[index].reset();
        }
    }
    
private:
    T pool[POOL_SIZE];
    bool used[POOL_SIZE];
};
```

---

## 5. 配置系统

### 5.1 数据结构

```cpp
struct CardConfig {
    char name[32];
    Background::Type backgroundType;
    uint8_t backgroundBrightness;
    
    struct ForegroundConfig {
        Foreground::Type type;
        Foreground::Position position;
        FontType font;
        uint8_t brightness;
        uint16_t displayDuration;  // 秒，0=不切换
    };
    
    ForegroundConfig foregrounds[MAX_FOREGROUNDS];
    uint8_t foregroundCount;
    
    bool autoSwitchForegrounds;
};

struct DisplayEngineConfig {
    CardConfig cards[MAX_CARDS];
    uint8_t cardCount;
    
    uint8_t currentCardIndex;
    bool autoSwitchCards;
    uint16_t cardSwitchInterval;  // 秒
    
    TransitionType transitionType;
    uint16_t transitionDuration;  // ms
    
    uint8_t globalBrightness;
};
```

### 5.2 JSON格式

```json
{
  "cards": [
    {
      "name": "时钟卡片",
      "background": {
        "type": "matrix_rain",
        "brightness": 128
      },
      "foregrounds": [
        {
          "type": "clock",
          "format": "HH_MM",
          "position": "center",
          "font": "5x7",
          "brightness": 255,
          "duration": 5
        },
        {
          "type": "temp_humid",
          "position": "bottom_right",
          "font": "3x5",
          "brightness": 200,
          "duration": 3
        }
      ],
      "autoSwitchForegrounds": true
    }
  ],
  "currentCard": 0,
  "autoSwitchCards": true,
  "cardSwitchInterval": 30,
  "transition": {
    "type": "fade",
    "duration": 500
  },
  "globalBrightness": 150
}
```

---

## 6. 实施顺序

### Week 3.1: 基础框架
1. [ ] 实现Background基类
2. [ ] 实现Foreground基类
3. [ ] 实现Transition类
4. [ ] 实现Card类

### Week 3.2: 背景效果
1. [ ] FireBackground（火焰）
2. [ ] MatrixRainBackground（矩阵雨）
3. [ ] WaterRippleBackground（水波纹）
4. [ ] GameOfLifeBackground（生命游戏）

### Week 4.1: 更多背景
1. [ ] SandBackground（沙漏）
2. [ ] PongBackground（Pong时钟）
3. [ ] WeatherBackground（天气系统）

### Week 4.2: 前景效果
1. [ ] ClockForeground（时钟）
2. [ ] DateForeground（日期）
3. [ ] TempForeground（温湿度）

### Week 5.1: 管理器
1. [ ] DisplayManager实现
2. [ ] 按键集成
3. [ ] 配置系统

### Week 5.2: 集成测试
1. [ ] 所有效果集成测试
2. [ ] 性能优化
3. [ ] MQTT配置更新
4. [ ] 文档完善

---

## 7. 测试策略

### 7.1 单元测试

```cpp
// test_backgrounds.cpp
void test_fire_background() {
    FireBackground fire;
    LEDMatrix matrix;
    
    fire.init();
    fire.render(matrix);
    
    // 验证火焰不为空
    bool hasFire = false;
    for (uint8_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            if (matrix.getPixel(x, y) > 0) {
                hasFire = true;
                break;
            }
        }
    }
    assert(hasFire);
}
```

### 7.2 集成测试

```cpp
// test_display_manager.cpp
void test_card_switching() {
    DisplayManager& dm = DisplayManager::getInstance();
    
    Card* card1 = new Card("Card1");
    card1->setBackground(new FireBackground());
    
    Card* card2 = new Card("Card2");
    card2->setBackground(new MatrixRainBackground());
    
    dm.addCard(card1);
    dm.addCard(card2);
    
    dm.switchCard(0);
    assert(dm.getCurrentCard() == card1);
    
    dm.nextCard();
    assert(dm.getCurrentCard() == card2);
}
```

### 7.3 性能基准

| 指标 | 目标 | 测试方法 |
|------|------|----------|
| 帧率 | >= 30fps | 统计每秒render()调用次数 |
| 内存使用 | < 20KB | 使用ESP.getFreeHeap() |
| 切换延迟 | < 100ms | 测量switchCard()耗时 |

---

**下一步**: 创建Phase 3天气系统设计
