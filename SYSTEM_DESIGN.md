# LED矩阵时钟 - 系统架构设计文档

> **版本**: v2.0  
> **日期**: 2026-03-05  
> **状态**: 设计阶段  
> **作者**: A-nemon-e

---

## 目录

1. [项目概述与架构](#1-项目概述与架构)
2. [硬件架构](#2-硬件架构)
3. [显示引擎设计](#3-显示引擎设计)
4. [天气系统](#4-天气系统)
5. [红外控制系统](#5-红外控制系统)
6. [后端服务架构](#6-后端服务架构)
7. [前端应用设计](#7-前端应用设计)
8. [API接口规范](#8-api接口规范)
9. [OTA升级系统](#9-ota升级系统)
10. [开发计划与里程碑](#10-开发计划与里程碑)

---

## 1. 项目概述与架构

### 1.1 项目目标

构建一个基于ESP8266的多功能LED矩阵显示时钟系统，集成：
- **灵活显示系统**: 前景+背景分层叠加，支持多种组合
- **智能天气**: 自动定位+多源天气数据，支持6种天气效果
- **红外控制**: 保留现有空调红外控制功能
- **云端管理**: 完整的用户系统、设备管理、OTA升级

### 1.2 系统架构总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                           用户层 (前端)                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │  显示设置   │ │  设备管理   │ │  用户中心   │ │  管理后台   │   │
│  │  卡片配置   │ │  绑定/解绑  │ │  注册/登录  │ │  用户管理   │   │
│  │  天气设置   │ │  OTA升级    │ │  修改密码   │ │  设备监控   │   │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ HTTP/WebSocket
┌─────────────────────────────────────────────────────────────────────┐
│                           服务层 (后端)                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │  用户服务   │ │  设备服务   │ │  天气服务   │ │  OTA服务    │   │
│  │  Auth/JWT   │ │  绑定管理   │ │  IP定位     │ │  固件管理   │   │
│  │  注册/登录  │ │  MQTT转发   │ │  和风API    │ │  版本控制   │   │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘   │
│  ┌─────────────┐ ┌─────────────┐                                    │
│  │  数据库     │ │  MQTT Broker│                                    │
│  │  SQLite     │ │  Mosquitto  │                                    │
│  └─────────────┘ └─────────────┘                                    │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ MQTT
┌─────────────────────────────────────────────────────────────────────┐
│                           硬件层 (ESP8266)                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │  显示引擎   │ │  天气渲染   │ │  红外控制   │ │  网络管理   │   │
│  │  前景/背景  │ │  6种效果    │ │  1收1发     │ │  WiFi/MQTT  │   │
│  │  卡片系统   │ │  自动切换   │ │  学习/发射  │ │  OTA升级    │   │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘   │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                      硬件抽象层                              │   │
│  │  IS31FL3733 x6 │ AHT20 │ IR Send/Recv │ Button │ Status LED │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.3 技术栈

| 层级 | 技术 | 说明 |
|------|------|------|
| **硬件** | ESP-12F (ESP8266) | 主控芯片，4MB Flash |
| **驱动芯片** | IS31FL3733 x6 | LED矩阵驱动，I2C接口 |
| **传感器** | AHT20 | 温湿度传感器 |
| **红外** | IRremoteESP8266 | 红外编解码库 |
| **后端** | NestJS + TypeScript | Node.js框架，支持TypeORM |
| **数据库** | SQLite | 轻量级关系型数据库 |
| **消息队列** | MQTT (Mosquitto) | 设备与服务器通信 |
| **前端** | Vue 3 + Vite + TypeScript | 现代化前端框架 |
| **UI组件** | Vant 4 | 移动端UI组件库 |
| **天气API** | 和风天气 | 免费版支持1000次/天 |

---

## 2. 硬件架构

### 2.1 引脚定义 (ESP-12F)

| GPIO | 功能 | 方向 | 电平 | 备注 |
|:----:|------|:----:|:----:|------|
| **IO0** | SDB | 输出 | 3.3V | IS31FL3733 Shutdown控制，低电平复位 |
| **IO1** | TX | 输出 | 3.3V | 串口调试输出 |
| **IO2** | IR_RECV_1 | 输入 | 3.3V | 1838B红外接收器 #1 (当前使用) |
| **IO3** | RX | 输入 | 3.3V | 串口输入 |
| **IO4** | SDA | 双向 | 3.3V | I2C数据线 (10kΩ上拉) |
| **IO5** | SCL | 输出 | 3.3V | I2C时钟线 (10kΩ上拉) |
| **IO12** | IR_RECV_2 | 输入 | 3.3V | 1838B红外接收器 #2 (**暂缓开发**) |
| **IO13** | IR_RECV_3 | 输入 | 3.3V | 1838B红外接收器 #3 (**暂缓开发**) |
| **IO14** | IR_SEND | 输出 | 3.3V | 红外发射驱动 (SS8050) |
| **IO15** | LED_SYS | 输出 | 3.3V | 系统状态LED |
| **IO16** | BUTTON | 输入 | 3.3V | 微动开关 (**高电平有效**，按通=3.3V) |
| **ADC** | - | - | - | 悬空未使用 |

### 2.2 I2C设备地址分配

| 设备 | I2C地址 | 说明 |
|------|:-------:|------|
| AHT20 | 0x38 | 温湿度传感器 |
| IS31FL3733 #0 | 0x50 | LED驱动，ADDR1=GND, ADDR2=GND |
| IS31FL3733 #1 | 0x51 | LED驱动，ADDR1=SCL, ADDR2=GND |
| IS31FL3733 #2 | 0x52 | LED驱动，ADDR1=SDA, ADDR2=GND |
| IS31FL3733 #3 | 0x53 | LED驱动，ADDR1=VCC, ADDR2=GND |
| IS31FL3733 #4 | 0x54 | LED驱动，ADDR1=GND, ADDR2=SCL |
| IS31FL3733 #5 | 0x55 | LED驱动，ADDR1=SCL, ADDR2=SCL |

### 2.3 LED矩阵布局

**总分辨率**: 42列 × 11行 = 462个LED

```
芯片 drv0 (0x50)    芯片 drv1 (0x51)    ...    芯片 drv5 (0x55)
列 0-6              列 7-13                    列 35-41
┌─────────────────┐ ┌─────────────────┐        ┌─────────────────┐
│ 0   1   2   3   │ │ 7   8   9   10  │        │ 35  36  37  38  │  Row 0
│ 4   5   6   7   │ │ 11  12  13  14  │        │ 39  40  41  42  │  Row 1
│ 8   9   10  11  │ │ 15  16  17  18  │        │ 43  44  45  46  │  Row 2
│ ...             │ │ ...             │        │ ...             │  ...
│ 70  71  72  73  │ │ 77  78  79  80  │        │ 105 106 107 108 │  Row 10
└─────────────────┘ └─────────────────┘        └─────────────────┘
```


**每个3733实际使用**: 7路- × 11路+

---

## 3. 显示引擎设计

### 3.1 核心概念

#### 3.1.1 分层架构

显示系统采用**三层架构**：

```
┌────────────────────────────────────────┐
│            Layer 3: 前景层             │  ← 时钟、日期、温湿度
│         (Foreground Layer)             │     可叠加、可切换
├────────────────────────────────────────┤
│            Layer 2: 背景层             │  ← 火焰、矩阵雨、天气效果
│         (Background Layer)             │     全屏动态效果
├────────────────────────────────────────┤
│            Layer 1: 基础层             │  ← 清屏、亮度控制
│          (Base Layer)                  │
└────────────────────────────────────────┘
```

#### 3.1.2 前景 (Foreground)

**类型**: 信息显示，可叠加在背景上

| 前景类型 | 说明 | 可选字体 | 默认字体 |
|----------|------|----------|----------|
| **时钟** | HH:MM 或 HH:MM:SS | 3×5, 3×9, 5×7, 5×5, 6×9 | 5*7 |
| **日期星期** | MM-DD DAY | 3×5, 5×7, 3x9 | 3×5 |
| **温湿度** | T 25.3  H 45 | 3×5, 5×7 | 3×5 |

**前景特性**:
- **可叠加性**: 多个前景可按可设置秒数自动切换（如：3秒时钟→2秒温湿度）
- **位置可调**: 不同背景支持不同前景位置（见3.3.3）
- **独立字体**: 每种卡片独立设置字体大小
- **独立亮度**: 每个前景可调亮度

#### 3.1.3 背景 (Background)

**类型**: 全屏动态效果或静态图案

| 背景类型 | 类别 | 可叠加前景 | 特殊限制 |
|----------|------|:----------:|----------|
| **火焰** | 天气(≥30°C) | ✅ | 只支持3×5字体，位置靠上 |
| **矩阵雨** | 特效 | ✅ | 前景自动清除雨滴区域 |
| **水波纹** | 特效 | ❌ | 全屏动画，不可叠加 |
| **生命游戏** | 特效 | ❌ | 全屏细胞自动机 |
| **沙漏模拟** | 特效 | ❌ | 全屏物理模拟 |
| **晴天** | 天气 | ✅ | 前景靠右避开太阳 |
| **下雨** | 天气 | ✅ | 大/中/小雨自动切换 |
| **刮风** | 天气 | ✅ | - |
| **下雪** | 天气 | ✅ | - |
| **阴天** | 天气 | ✅ | - |

**背景特性**:
- **全屏渲染**: 背景占据整个42×11像素区域
- **动态更新**: 每帧更新（FPS由背景类型决定，通常30FPS）
- **独立亮度**: 每个背景可调亮度

#### 3.1.4 卡片 (Card)

**定义**: 卡片是用户可见的显示模式，由**0-1个背景** + **0-N个前景**组成

**示例卡片**:
```
卡片0: 纯时钟
  - 背景: 无(黑屏)
  - 前景: [时钟]
  - 字体: 3×9

卡片1: 火焰时钟
  - 背景: 火焰(≥30°C触发)
  - 前景: [时钟(3秒), 温湿度(2秒)]
  - 字体: 3×5 (火焰背景限制)
  - 位置: 靠上

卡片2: 矩阵雨时钟
  - 背景: 矩阵雨
  - 前景: [时钟, 日期]
  - 字体: 5×7

卡片3: 水波纹
  - 背景: 水波纹
  - 前景: 无
  - 说明: 纯背景效果，不可叠加前景

卡片4: 天气时钟
  - 背景: 天气系统自动选择
  - 前景: [时钟, 日期星期]
  - 位置: 选择道不同天气的时候根据这个天气自动选择符合的前景位置
```

### 3.2 架构设计

#### 3.2.1 类图

```
┌─────────────────────────────────────────────────────────────────┐
│                     DisplayManager (显示管理器)                  │
│  - currentCard: Card                                             │
│  - cards: Card[]                                                 │
│  - autoSwitch: bool                                              │
│  - autoSwitchInterval: int                                       │
│  + switchCard(index)                                             │
│  + nextCard()                                                    │
│  + render()                                                      │
└──────────────────────────┬────────────────────────────────────────┘
                           │ uses
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                          Card (卡片)                             │
│  - background: Background|null                                   │
│  - foregrounds: Foreground[]                                     │
│  - fgSwitchInterval: int (前景切换间隔秒)                         │
│  - transitionType: TransitionType                                │
│  + render()                                                      │
│  + switchForeground()                                            │
└────────────┬──────────────────────────────┬─────────────────────┘
             │ inherits                     │ inherits
             ▼                              ▼
┌──────────────────────────┐  ┌──────────────────────────────┐
│    Background (背景基类)  │  │    Foreground (前景基类)      │
│  - brightness: int       │  │  - fontSize: FontSize        │
│  - transition: Transition│  │  - position: Position        │
│  + render() [virtual]    │  │  - brightness: int           │
│  + update() [virtual]    │  │  + render() [virtual]        │
└────────────┬─────────────┘  └──────────────┬───────────────┘
             │ inherits                      │ inherits
             ▼                               ▼
    ┌──────────────────┐          ┌──────────────────┐
    │ FireBackground   │          │ ClockForeground  │
    │ RainBackground   │          │ DateForeground   │
    │ MatrixBackground │          │ TempForeground   │
    │ RippleBackground │          │ WeekForeground   │
    │ LifeBackground   │          └──────────────────┘
    │ SandBackground   │
    │ SunBackground    │
    │ SnowBackground   │
    │ WindBackground   │
    │ CloudBackground  │
    └──────────────────┘
```

#### 3.2.2 渲染流程

```cpp
// 主循环
void loop() {
    displayManager.update();  // 更新显示
    delay(1000 / FPS);        // 控制帧率
}

// DisplayManager::update()
void DisplayManager::update() {
    // 1. 检查是否需要切换卡片
    if (autoSwitch && millis() - lastCardSwitch > cardInterval) {
        nextCard();
    }
    
    // 2. 检查是否需要切换前景
    if (currentCard->fgSwitchInterval > 0 && 
        millis() - lastFgSwitch > currentCard->fgSwitchInterval * 1000) {
        currentCard->switchForeground();
    }
    
    // 3. 执行渲染
    render();
}

// DisplayManager::render()
void DisplayManager::render() {
    // 3.1 渲染背景
    if (currentCard->background) {
        currentCard->background->render(frameBuffer);
    } else {
        frameBuffer.clear();
    }
    
    // 3.2 渲染前景（叠加）
    for (auto fg : currentCard->currentForegrounds) {
        fg->render(frameBuffer);
    }
    
    // 3.3 应用转场效果
    if (inTransition) {
        applyTransition(frameBuffer);
    }
    
    // 3.4 写入硬件
    writeToLEDs(frameBuffer);
}
```

### 3.3 转场效果系统

#### 3.3.1 转场类型

```cpp
enum class TransitionType {
    NONE,           // 硬切（无动画）
    FADE,           // 淡入淡出
    SLIDE_LEFT,     // 向左滑入
    SLIDE_RIGHT,    // 向右滑入
    SLIDE_UP,       // 向上滑入
    SLIDE_DOWN,     // 向下滑入
    WIPE,           // 擦除效果
    DISSOLVE,       // 溶解效果
    PIXELATE        // 像素化（预留）
};
```

#### 3.3.2 前景切换转场

- **默认**: 硬切（快速切换）
- **可选**: 淡入淡出（推荐，200ms）
- **独立配置**: 每个卡片可独立设置前景转场效果

#### 3.3.3 卡片切换转场

- **默认**: 淡入淡出（300ms）
- **可选**: 滑动（方向可选）
- **独立配置**: 每个卡片可独立设置卡片转场效果

### 3.4 字体系统

#### 3.4.1 字体定义

```cpp
enum class FontSize {
    FONT_3x5,   // 3列×5行，小字体
    FONT_3x9,   // 3列×9行，高字体
    FONT_5x5,   // 5列×5行，中方字体
    FONT_5x7,   // 5列×7行，标准字体
    FONT_6x9    // 6列×9行，大数字
};

struct Font {
    FontSize size;
    uint8_t width;
    uint8_t height;
    const uint8_t (*data)[HEIGHT];  // 字形数据
};
```

#### 3.4.2 字体选择器（预留，低优先级）

- **功能**: 用户通过点击像素设计自定义字体
- **实现**: 前端提供像素编辑器，生成字形数据
- **传输**: 通过MQTT/HTTP下发到ESP
- **存储**: 保存到EEPROM或SPIFFS

### 3.5 位置与布局

#### 3.5.1 背景特定的前景位置

| 背景 | 前景位置 | 说明 |
|------|----------|------|
| **火焰** | 顶部居中 (y=0-5) | 火焰占下半部分 |
| **矩阵雨** | 居中 | 自动清除雨滴 |
| **晴天** | 右侧 (x=21-41) | 避开左侧太阳 |
| **其他** | 居中 (x=10-31) | 默认居中 |

#### 3.5.2 亮度控制

- **全局亮度**: IS31FL3733的GCC寄存器（0-255）
- **背景独立亮度**: 每个背景类可覆盖亮度值
- **前景独立亮度**: 每个前景类可覆盖亮度值
- **用户调节**: 按键长按调节，前端也可设置

---

## 4. 天气系统

### 4.1 天气效果定义

采用**方案C（混合方案）**: 服务器中转 + 自动定位 + 手动设置

#### 4.1.1 天气类型

| 天气代码 | 名称 | 触发条件 | 视觉效果 | 可叠加前景 |
|:--------:|------|----------|----------|:----------:|
| **fire** | 火焰 | 温度≥30°C | DOOM火焰算法 | ✅ (3×5字体，靠上) |
| **rain_heavy** | 大雨 | API返回大雨 | 高密度雨滴+闪电 | ✅ |
| **rain_medium** | 中雨 | API返回中雨 | 中密度雨滴 | ✅ |
| **rain_light** | 小雨 | API返回小雨 | 低密度雨滴 | ✅ |
| **snow** | 下雪 | API返回雪 | 雪花飘落 | ✅ |
| **wind** | 刮风 | API返回大风 | 风沙/飘动物体 | ✅ |
| **sun** | 晴天 | API返回晴 | 太阳图标 | ✅ (靠右) |
| **cloud** | 阴天 | API返回多云/阴 | 云层 | ✅ |

**注**: 下雨效果demo（test3733scanner）已实现大/中/小雨自动切换（demo每5秒切换，参数直接利用。虽然实际实现要求要根据天气系统来，但是每种大小的参数你直接利用）

### 4.2 系统架构

```
┌──────────────────────────────────────────────────────────────┐
│                     天气系统数据流                            │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│   ESP8266          MQTT           Server          和风API   │
│      │              │                │              │       │
│      │ 1. 启动      │                │              │       │
│      │──────────────▶                │              │       │
│      │              │ 上报IP         │              │       │
│      │              │────────────────▶              │       │
│      │              │                │ 2. IP定位     │       │
│      │              │                │──────────────▶       │
│      │              │                │ 返回城市坐标  │       │
│      │              │                │◀──────────────       │
│      │              │ 推送位置       │              │       │
│      │              │◀───────────────│              │       │
│      │              │                │              │       │
│      │              │                │ 3. 定时查询   │       │
│      │              │                │ (每10分钟)   │       │
│      │              │                │──────────────▶       │
│      │              │                │ 返回天气JSON  │       │
│      │              │                │◀──────────────       │
│      │              │                │              │       │
│      │              │ 4. 推送天气     │              │       │
│      │              │◀───────────────│              │       │
│      │              │                │              │       │
│      │ 5. 渲染      │                │              │       │
│      │ 天气效果      │                │              │       │
│      ▼              │                │              │       │
│   LED Matrix        │                │              │       │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 4.3 服务器端设计

#### 4.3.1 服务模块

```typescript
// WeatherService (NestJS)
@Injectable()
export class WeatherService {
    // IP定位
    async locateByIP(ip: string): Promise<Location> {
        // 调用IP定位API（如ip-api.com、淘宝IP库等）
        // 返回: { city: string, lat: number, lon: number }
    }
    
    // 查询天气
    async fetchWeather(location: Location): Promise<WeatherData> {
        // 调用和风天气API
        // 缓存结果10分钟
        // 返回标准化天气数据
    }
    
    // 推送到设备
    async pushToDevice(deviceId: string, weather: WeatherData): Promise<void> {
        // 通过MQTT推送
        // Topic: ac/user_{uid}/dev_{uuid}/weather/update
    }
}
```

#### 4.3.2 数据结构

```typescript
// 天气数据（服务器→ESP）
interface WeatherPayload {
    condition: 'fire' | 'rain_heavy' | 'rain_medium' | 'rain_light' 
              | 'snow' | 'wind' | 'sun' | 'cloud';
    temperature: number;      // 当前温度
    humidity: number;         // 当前湿度
    location: string;         // 城市名称
    updateTime: string;       // ISO 8601时间
    forecast?: WeatherItem[]; // 未来3天预报（预留）
}

// 位置信息
interface Location {
    city: string;      // 城市名，如"北京"
    lat?: number;      // 纬度（可选）
    lon?: number;      // 经度（可选）
    source: 'auto' | 'manual';  // 定位来源
}
```

#### 4.3.3 缓存策略

- **天气缓存**: 15分钟，减少API调用
- **位置缓存**: 硬件重新启动时更新
- **降级策略**: API失败时使用缓存数据，缓存过期后显示"未知天气"

### 4.4 ESP端设计

#### 4.4.1 天气状态机

```cpp
enum class WeatherState {
    UNKNOWN,        // 未获取天气
    FETCHING,       // 等待服务器推送
    ACTIVE,         // 正常显示天气
    OFFLINE         // 天气功能关闭
};

class WeatherManager {
    WeatherState state;
    WeatherData currentWeather;
    unsigned long lastUpdate;
    
public:
    void onWeatherUpdate(const char* json);  // MQTT回调
    Background* createWeatherBackground();   // 创建对应背景
    bool isEnabled();                        // 是否启用天气
};
```

#### 4.4.2 自动 vs 手动位置

- **自动模式**（默认）:
  - ESP启动时上报公网IP（设置2-3 api防止失效）
  - 服务器解析IP获取城市
  - 自动更新天气
  
- **手动模式**:
  - 用户在前端输入城市名
  - 服务器保存到设备配置
  - 使用该城市查询天气

### 4.5 前端设置

```
天气设置页面
├── 启用天气功能 [开关]
├── 位置设置
│   ├── 自动定位 (推荐) [单选]
│   │   └── 当前位置: 北京市 [显示]
│   ├── 手动设置 [单选]
│   │   └── 城市输入: [________] [保存]
│   └── 更新频率: [15分钟（最小） ▼]
├── 天气效果预览
│   └── [太阳图标] 晴天 25°C
└── 高级设置
    └── 温度阈值: [30°C] 以上显示火焰效果
```

---

## 5. 红外控制系统

### 5.1 现状与复用

**当前状态**: 已实现1收1发，功能完整

**复用策略**: 完全保留现有代码，不做修改

### 5.2 现有功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 红外学习 | ✅ 已实现 | 学习任意红外信号原始数据 |
| 品牌协议 | ✅ 已实现 | 支持GREE、MIDEA等主流品牌 |
| 发射控制 | ✅ 已实现 | 支持原始数据/品牌协议 |
| 回声过滤 | ✅ 已实现 | 防止自发自收 |
| MQTT集成 | ✅ 已实现 | 远程控制/学习 |

### 5.3 暂缓开发：3收1发

**需求**: 3个红外接收器（IO2、IO12、IO13）360°接收

**状态**: 暂缓，当前只使用IO2

**未来扩展**:
```cpp
// 预留多接收器支持
class IRController {
    static IRrecv irrecv1;  // IO2 - 已启用
    static IRrecv irrecv2;  // IO12 - 预留
    static IRrecv irrecv3;  // IO13 - 预留
    
    // 合并接收结果，去重
    static void handleMultiReceive();
};
```

### 5.4 API接口（保留现有）

```cpp
// 红外控制
POST /devices/{id}/cmd           // 发送控制命令
POST /devices/{id}/learn/start   // 开始学习
GET  /devices/{id}/learn/status  // 获取学习状态
DELETE /devices/{id}/learn/{key} // 删除学习码

// MQTT Topic
ac/user_{uid}/dev_{uuid}/cmd           // 接收命令
ac/user_{uid}/dev_{uuid}/learn/start   // 开始学习
ac/user_{uid}/dev_{uuid}/learn/result  // 学习结果
ac/user_{uid}/dev_{uuid}/ir_event      // 红外接收事件
```

---

## 6. 后端服务架构

### 6.1 现有服务复用与改进

#### 6.1.1 设备绑定逻辑改进

**现有问题**:
1. 设备重复绑定逻辑不够健壮
2. 意外掉绑定处理可能不完善
3. 缺乏绑定状态同步机制

**改进方案**:

```typescript
// DeviceBindingService
@Injectable()
export class DeviceBindingService {
    // 改进的绑定逻辑
    async bindDevice(userId: number, deviceDto: CreateDeviceDto): Promise<Device> {
        const existing = await this.findByUUID(deviceDto.uuid);
        
        if (existing) {
            if (existing.userId === userId) {
                // 已绑定到自己，更新信息
                return this.updateDevice(existing.id, deviceDto);
            } else if (existing.userId !== 0) {
                // 已绑定到他人，拒绝或支持强制解绑
                throw new ConflictException('Device already bound to another user');
            }
            // userId=0表示未绑定，允许绑定
        }
        
        // 创建或更新绑定
        return this.createOrUpdateBinding(userId, deviceDto);
    }
    
    // 心跳检测绑定状态
    async checkDeviceHealth(uuid: string): Promise<DeviceHealth> {
        // 检查设备最后在线时间
        // 检查MQTT连接状态
        // 返回设备健康状态
    }
    
    // 解绑时重置设备
    async unbindDevice(userId: number, deviceId: number): Promise<void> {
        const device = await this.findOne(userId, deviceId);
        
        // 1. 数据库解绑
        await this.devicesRepository.delete(deviceId);
        
        // 2. 发送解绑命令到设备（重置userId=0）
        await this.mqttService.publish(
            `ac/user_${userId}/dev_${device.uuid}/config/update`,
            JSON.stringify({ userId: 0, deviceId: 0 })
        );
        
        // 3. 记录审计日志
        await this.auditLogRepository.save({
            deviceId,
            action: 'unbind',
            userId,
            timestamp: new Date()
        });
    }
}
```

#### 6.1.2 绑定状态同步

```cpp
// ESP端: 定期检查绑定状态
void checkBindingStatus() {
    // 每5分钟检查一次
    if (millis() - lastBindingCheck > 5 * 60 * 1000) {
        mqttClient.publish(
            "ac/status/binding_check",
            json({
                uuid: deviceUUID,
                userId: currentUserId,
                timestamp: now()
            })
        );
        lastBindingCheck = millis();
    }
}

// 服务器端: 检查并纠正状态不一致
@OnEvent('device.binding_check')
async handleBindingCheck(payload: BindingCheckDto) {
    const device = await this.devicesRepository.findOne({
        where: { uuid: payload.uuid }
    });
    
    if (device && device.userId !== payload.userId) {
        // 状态不一致，推送正确配置
        await this.pushDeviceConfig(device);
    }
}
```

### 6.2 用户系统扩展

#### 6.2.1 用户注册（新增用户协议）

```typescript
// AuthController
@Post('register')
async register(@Body() dto: RegisterDto): Promise<AuthResponse> {
    // 1. 验证用户协议已同意
    if (!dto.agreedToTerms) {
        throw new BadRequestException('Must agree to terms');
    }
    
    // 2. 检查用户名/邮箱唯一性
    await this.validateUniqueUser(dto.username, dto.email);
    
    // 3. 密码强度验证
    this.validatePasswordStrength(dto.password);
    
    // 4. 创建用户
    const user = await this.authService.createUser(dto);
    
    // 5. 生成JWT
    const token = this.jwtService.sign({ userId: user.id });
    
    return { user, token };
}
```

#### 6.2.2 修改密码

```typescript
@Post('change-password')
@UseGuards(JwtAuthGuard)
async changePassword(
    @Request() req,
    @Body() dto: ChangePasswordDto
): Promise<{ success: boolean }> {
    // 1. 验证旧密码
    await this.authService.verifyPassword(req.user.userId, dto.oldPassword);
    
    // 2. 验证新密码强度
    this.validatePasswordStrength(dto.newPassword);
    
    // 3. 更新密码
    await this.authService.updatePassword(req.user.userId, dto.newPassword);
    
    return { success: true };
}
```

### 6.3 管理后台

#### 6.3.1 功能模块

```
管理后台 (Admin Panel)
├── 用户管理
│   ├── 用户列表 (分页、搜索)
│   ├── 用户详情
│   │   ├── 基本信息
│   │   ├── 绑定设备列表
│   │   └── 操作日志
│   ├── 禁用/启用用户
│   └── 重置用户密码
│
├── 设备管理
│   ├── 设备列表 (所有设备)
│   ├── 设备详情
│   │   ├── UUID、MAC地址
│   │   ├── 绑定用户
│   │   ├── WiFi连接状态
│   │   ├── WiFi名称 (SSID)
│   │   ├── 位置信息 (自动/手动)
│   │   ├── 最后在线时间
│   │   └── 固件版本
│   ├── 强制解绑
│   └── 远程重启
│
├── OTA管理
│   ├── 固件版本列表
│   ├── 上传新固件
│   ├── 推送OTA到设备
│   │   ├── 单设备推送
│   │   ├── 批量推送
│   │   └── 全部推送
│   └── OTA进度监控
│
└── 系统监控
    ├── 在线设备统计
    ├── MQTT连接状态
    └── 天气API调用统计
```

#### 6.3.2 管理员权限

```typescript
// 角色定义
enum UserRole {
    USER = 'user',      // 普通用户
    ADMIN = 'admin'     // 管理员
}

// AdminGuard
@Injectable()
export class AdminGuard implements CanActivate {
    canActivate(context: ExecutionContext): boolean {
        const request = context.switchToHttp().getRequest();
        const user = request.user;
        
        if (user.role !== UserRole.ADMIN) {
            throw new ForbiddenException('Admin access required');
        }
        
        return true;
    }
}

// 使用
@Controller('admin')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminController {
    // 只有管理员可访问
}
```

---

## 7. 前端应用设计

### 7.1 页面结构

```
App
├── 登录页 (/login)
├── 注册页 (/register)
│   └── 用户协议弹窗
│
├── 主布局 (需登录)
│   ├── Tab 1: 显示设置 (/display)
│   │   ├── 卡片列表
│   │   │   └── 卡片项 (可编辑/删除)
│   │   ├── 添加卡片
│   │   │   ├── 选择背景
│   │   │   ├── 选择前景 (可多选)
│   │   │   ├── 设置字体大小
│   │   │   └── 设置切换间隔
│   │   ├── 自动切换设置
│   │   │   ├── 启用自动切换
│   │   │   └── 切换间隔
│   │   └── 转场效果设置
│   │       ├── 前景转场
│   │       └── 卡片转场
│   │
│   ├── Tab 2: 天气设置 (/weather)
│   │   ├── 启用天气功能
│   │   ├── 位置设置
│   │   │   ├── 自动定位
│   │   │   └── 手动设置城市
│   │   └── 天气预览
│   │
│   ├── Tab 3: 设备管理 (/devices)
│   │   ├── 我的设备列表
│   │   │   └── 设备卡片 (状态/WiFi/位置)
│   │   ├── 添加设备
│   │   └── 设备详情
│   │       ├── 实时状态
│   │       ├── 传感器数据
│   │       └── OTA升级
│   │
│   └── Tab 4: 我的 (/profile)
│       ├── 用户信息
│       ├── 修改密码
│       └── 退出登录
│
└── 管理后台 (/admin)
    ├── 用户管理
    ├── 设备管理
    └── OTA管理
```

### 7.2 显示设置页面详细设计

```
┌─────────────────────────────────────────┐
│ 显示设置                     [保存]      │
├─────────────────────────────────────────┤
│                                         │
│ 我的卡片                                │
│ ┌───────────────────────────────────┐  │
│ │ 卡片1: 火焰时钟              [编辑]│  │
│ │ 背景: 火焰 | 前景: 时钟→温湿度    │  │
│ │ 字体: 3×5 | 切换: 3秒            │  │
│ └───────────────────────────────────┘  │
│ ┌───────────────────────────────────┐  │
│ │ 卡片2: 矩阵雨              [编辑]  │  │
│ │ 背景: 矩阵雨 | 前景: 时钟          │  │
│ │ 字体: 5×7 | 切换: 关闭           │  │
│ └───────────────────────────────────┘  │
│ ┌───────────────────────────────────┐  │
│ │ 卡片3: 水波纹              [编辑]  │  │
│ │ 背景: 水波纹 | 前景: 无           │  │
│ │ (纯背景，不可叠加前景)            │  │
│ └───────────────────────────────────┘  │
│ [+ 添加新卡片]                          │
│                                         │
│ 自动切换设置                            │
│ ┌───────────────────────────────────┐  │
│ │ 启用自动切换卡片        [开关]    │  │
│ │ 切换间隔: [30 ▼] 秒               │  │
│ │ (0=手动切换)                      │  │
│ └───────────────────────────────────┘  │
│                                         │
│ 转场效果                                │
│ ┌───────────────────────────────────┐  │
│ │ 前景切换效果: [淡入淡出 ▼]        │  │
│ │ 卡片切换效果: [滑动 ▼]            │  │
│ │ 切换速度: [正常 ▼]                │  │
│ └───────────────────────────────────┘  │
│                                         │
└─────────────────────────────────────────┘
```

### 7.3 添加卡片弹窗

```
┌─────────────────────────────────────────┐
│ 添加新卡片                      [X]      │
├─────────────────────────────────────────┤
│                                         │
│ 1. 选择背景 (0-1个)                     │
│ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐   │
│ │无  │ │火焰│ │矩阵│ │水波│ │生命│   │
│ │    │ │🔥  │ │🌧️  │ │💧  │ │🎮  │   │
│ └────┘ └────┘ └────┘ └────┘ └────┘   │
│ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐   │
│ │沙漏│ │晴天│ │下雨│ │刮风│ │下雪│   │
│ │⏳  │ │☀️  │ │🌧️  │ │💨  │ │❄️  │   │
│ └────┘ └────┘ └────┘ └────┘ └────┘   │
│                                         │
│ 已选: 火焰 (⚠️ 此背景只支持3×5字体)      │
│                                         │
│ 2. 选择前景 (0-多个)                    │
│ ☑ 时钟   ☑ 温湿度   ☐ 日期   ☐ 星期  │
│                                         │
│ 3. 前景设置                             │
│ ┌───────────────────────────────────┐  │
│ │ 字体大小: [3×5 ▼]                 │  │
│ │ 自动切换: [☑ 启用]               │  │
│ │ 切换间隔: [3 ▼] 秒                │  │
│ │ 顺序: 时钟 → 温湿度               │  │
│ │ 亮度: [████░░░░░░] 120/255        │  │
│ └───────────────────────────────────┘  │
│                                         │
│ 4. 预览                                 │
│ ┌───────────────────────────────────┐  │
│ │ [火焰动画 + 小字体时钟预览]        │  │
│ └───────────────────────────────────┘  │
│                                         │
│        [取消]              [保存]       │
└─────────────────────────────────────────┘
```

---

## 8. API接口规范

### 8.1 设备管理接口

```yaml
# 设备绑定
POST /api/devices
Request:
  uuid: string        # 设备UUID
  name: string        # 设备名称
  mac?: string       # MAC地址
Response:
  id: number
  uuid: string
  name: string
  userId: number
  createdAt: string

# 获取设备列表
GET /api/devices
Response:
  devices: [
    {
      id: number
      uuid: string
      name: string
      online: boolean
      lastSeen: string
      wifiStatus?: {
        connected: boolean
        ssid: string
        rssi: number
      }
      location?: {
        city: string
        source: 'auto' | 'manual'
      }
    }
  ]

# 获取设备详情
GET /api/devices/{id}
Response:
  id: number
  uuid: string
  name: string
  userId: number
  config: object      # 设备配置
  online: boolean
  lastSeen: string
  wifiStatus: object
  location: object
  firmwareVersion: string

# 删除设备（解绑）
DELETE /api/devices/{id}
Response: { success: true }

# 更新设备配置（显示设置）
PATCH /api/devices/{id}/display-config
Request:
  cards: CardConfig[]
  autoSwitch: boolean
  autoSwitchInterval: number
  foregroundTransition: TransitionType
  cardTransition: TransitionType
Response: { success: true }

# 更新天气设置
PATCH /api/devices/{id}/weather-config
Request:
  enabled: boolean
  locationSource: 'auto' | 'manual'
  manualCity?: string
Response: { success: true }
```

### 8.2 天气接口

```yaml
# 获取当前天气（服务器主动推送，无需设备轮询）
# MQTT Topic: ac/user_{uid}/dev_{uuid}/weather/update
Payload:
  condition: string    # fire/rain_heavy/rain_medium/rain_light/snow/wind/sun/cloud
  temperature: number
  humidity: number
  location: string     # 城市名
  updateTime: string   # ISO 8601

# 手动触发天气更新（调试用）
POST /api/devices/{id}/weather/refresh
Response: { success: true }
```

### 8.3 MQTT Topic规范

```
# 设备→服务器
ac/discovery/{uuid}/hello           # 设备上线通知
ac/user_{uid}/dev_{uuid}/status     # 设备状态上报
ac/user_{uid}/dev_{uuid}/sensor     # 传感器数据
ac/user_{uid}/dev_{uuid}/ir_event   # 红外接收事件

# 服务器→设备
ac/user_{uid}/dev_{uuid}/config/update    # 配置更新
ac/user_{uid}/dev_{uuid}/cmd              # 控制命令
ac/user_{uid}/dev_{uuid}/weather/update   # 天气更新
ac/user_{uid}/dev_{uuid}/ota/start        # OTA升级通知

# 服务器广播
ac/broadcast/time_sync              # 时间同步
ac/broadcast/system_notice          # 系统通知
```

---

## 9. OTA升级系统

### 9.1 架构设计

```
┌──────────────┐      HTTP       ┌──────────────┐
│   管理后台    │ ───────────────▶ │  固件服务器   │
│  上传新固件   │                  │  (Nginx/本地) │
└──────────────┘                  └──────┬───────┘
                                         │
                    1. 通知OTA可用        │
                    MQTT: ota/start       │
                                         ▼
                                  ┌──────────────┐
                                  │    ESP8266   │
                                  │  1. 接收通知  │
                                  │  2. 下载固件  │◀── HTTP
                                  │  3. 校验签名  │
                                  │  4. 写入Flash │
                                  │  5. 重启设备  │
                                  └──────────────┘
```

### 9.2 安全机制

1. **版本校验**: 允许降级
2. **签名验证**: 固件需签名，防止刷入恶意固件
3. **断点续传**: 支持下载中断后恢复
4. **回滚机制**: 升级失败自动回滚到旧版本

### 9.3 流程

```cpp
// ESP端OTA流程
void handleOTA(const char* firmwareUrl, const char* expectedHash) {
    // 1. 验证版本
    if (!isNewerVersion(firmwareUrl)) {
        return;
    }
    
    // 2. 下载固件
    HTTPClient http;
    http.begin(firmwareUrl);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        // 3. 校验哈希
        String firmware = http.getString();
        if (calculateHash(firmware) != expectedHash) {
            reportOTAError("Hash mismatch");
            return;
        }
        
        // 4. 写入Flash
        if (Update.begin(firmware.length())) {
            Update.write((uint8_t*)firmware.c_str(), firmware.length());
            if (Update.end()) {
                // 5. 重启
                ESP.restart();
            }
        }
    }
}
```

---

## 10. 开发计划与里程碑

### 10.1 开发阶段

| 阶段 | 时间 | 内容 | 优先级 |
|------|------|------|:------:|
| **Phase 1** | Week 1-2 | 硬件抽象层重构 | P0 |
| | | - IS31FL3733驱动抽象 | |
| | | - 按键处理重构（IO16优化） | |
| | | - 基础渲染框架 | |
| **Phase 2** | Week 3-4 | 显示引擎核心 | P0 |
| | | - 前景/背景基类 | |
| | | - 卡片系统 | |
| | | - 转场效果 | |
| | | - 字体系统模块化 | |
| **Phase 3** | Week 5-6 | 天气系统 | P0 |
| | | - 服务器天气服务 | |
| | | - IP定位 | |
| | | - 6种天气效果 | |
| | | - 前端天气设置 | |
| **Phase 4** | Week 7-8 | 后端改进 | P1 |
| | | - 设备绑定健壮性 | |
| | | - 用户注册/登录 | |
| | | - 修改密码 | |
| | | - 管理后台 | |
| **Phase 5** | Week 9-10 | 前端完善 | P1 |
| | | - 显示设置页面 | |
| | | - 卡片配置器 | |
| | | - 设备管理 | |
| **Phase 6** | Week 11-12 | OTA系统 | P1 |
| | | - 固件上传 | |
| | | - OTA推送 | |
| | | - 进度监控 | |
| **Phase 7** | Week 13+ | 优化与测试 | P2 |
| | | - 性能优化 | |
| | | - 稳定性测试 | |
| | | - Bug修复 | |

### 10.2 优先级说明

- **P0**: 核心功能，必须完成
- **P1**: 重要功能，应该完成
- **P2**: 优化项，时间允许完成
- **P3**: 预留功能，后续迭代（如字体设置器）

### 10.3 风险与应对

| 风险 | 影响 | 应对策略 |
|------|------|----------|
| ESP8266内存不足 | 高 | 优化数据结构，使用PROGMEM，必要时升级ESP32 |
| 天气API限制 | 中 | 实现缓存，准备备用API源 |
| 显示效果卡顿 | 中 | 优化渲染循环，降低FPS，使用硬件PWM |
| 前端复杂度 | 中 | 分阶段开发，先做核心功能 |

---

## 附录

### A. 字体定义参考

```cpp
// 字体数据结构示例
static const uint8_t FONT_3x5_NUMBERS[10][5] PROGMEM = {
    {0x06, 0x05, 0x05, 0x05, 0x03},  // 0
    {0x02, 0x06, 0x02, 0x02, 0x07},  // 1
    // ...
};

static const uint8_t FONT_5x7_CHARS[96][7] PROGMEM = {
    // ASCII 32-127
};
```

### B. 颜色/亮度映射

IS31FL3733使用8位PWM (0-255):
- 0: 完全熄灭
- 1-10: 微亮（夜间模式）
- 11-50: 低亮
- 51-150: 中亮
- 151-255: 高亮（白天模式）

### C. 文档变更记录

| 版本 | 日期 | 变更内容 |
|------|------|----------|
| v1.0 | 2026-03-05 | 初始版本，完整系统设计 |

---

**文档结束**
