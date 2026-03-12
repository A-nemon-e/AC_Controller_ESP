# LED矩阵时钟 - 完整项目设计文档

> **版本**: v2.0  
> **日期**: 2026-03-05  
> **作者**: A-nemon-e  
> **状态**: 详细设计阶段

---

## 目录

1. [项目概述与目标](#1-项目概述与目标)
2. [系统架构总览](#2-系统架构总览)
3. [硬件抽象层设计](#3-硬件抽象层设计)
4. [引脚定义与连接](#4-引脚定义与连接)
5. [红外系统（复用现有）](#5-红外系统复用现有)
6. [显示引擎架构](#6-显示引擎架构)
7. [天气系统设计](#7-天气系统设计)
8. [转场效果系统](#8-转场效果系统)
9. [按键交互重构](#9-按键交互重构)
10. [字体系统](#10-字体系统)
11. [OTA升级系统](#11-ota升级系统)
12. [后端服务改进](#12-后端服务改进)
13. [前端界面设计](#13-前端界面设计)
14. [API接口规范](#14-api接口规范)
15. [数据模型](#15-数据模型)
16. [开发里程碑](#16-开发里程碑)

---

## 1. 项目概述与目标

### 1.1 项目背景

本项目是基于ESP-12F的多功能LED矩阵显示时钟，硬件配置为：
- ESP-12F主控
- 6个IS31FL3733 LED驱动芯片（42×11分辨率）
- AHT20温湿度传感器
- 红外收发系统（1收1发，预留3收1发扩展）
- 微动开关（IO16，高电平有效）

### 1.2 核心目标

**复用现有前后端代码**（完全保留已有红外部分，1收1发，3收1发逻辑暂缓开发但保留需求），在此基础上进行以下改进：

#### 显示系统重构
- 将显示模式分为**前景**和**背景**
- **前景**：时钟、温湿度、日期星期
- **背景**：火焰、代码雨（下雨）、水波纹、生命游戏、沙漏、Pong时钟、天气系统（晴/雨/雪/风/阴）
- **不可叠加前景的背景**：水波纹、沙漏、生命游戏（3个）
- 创建**卡片系统**：1个卡片 = 0至1个背景 + 0至N个前景的叠加
- 卡片可通过双击按键切换
- 前端支持自动切换卡片
- 多个前景自动切换可设置sec（如火焰时钟按seconds自动切换前景内容）
- 预留前景切换转场设置：硬切、淡入淡出、滑入滑出等
- 预留卡片转场设置：同上

#### 天气功能
- 天气作为独立背景，包含：火焰（30度及以上触发）、下雨（大中小雨5秒自动切换）、刮风、下雪、晴天（太阳，后期加月亮）、阴天
- 可显示1-多个天气组合（如刮风下雪、高温晴天等）
- ESP启动连接服务器时，通过任一方调用API将公网IP转为坐标或城市字符串
- 天气API调用时传入地理位置参数（坐标或城市字符串）
- 前端功能：是否启用天气、位置自动获取或用户手动输入城市

#### 字体系统
- 前景可更换字体大小
- 每个前景分别设置字体大小
- 后期引入更多字体（同分辨率或不同分辨率）
- 字体设置器（优先级最低）：用户通过点击像素设置字体，直接让硬件显示
- 代码中已定义多种字体，需要模块化

#### 约束与限制
- 卡片 = 0-1个背景 + 0-N个前景
- 每种卡片独立设置字体大小
- 不同前景和背景组合时位置不同：
  - 火焰背景：前景只支持3×5小字体，靠上（位置写死，不同字体大小时位置可能不同）
  - 晴天背景（太阳）：前景靠右避免重叠
- 每个背景和前景考虑是否提供亮度调节选项

#### 系统改进
- 重构按键处理（IO16不支持中断，优雅方案，沿用EasyButton）
- 服务端设备绑定逻辑健壮性改进（防重复绑定、防意外掉绑定）
- OTA在线升级（利用服务器完成）
- 用户系统：注册（加用户协议）、改密码
- 管理后台：查看管理用户、设备UUID、WiFi状态、位置、推送OTA

---

## 2. 系统架构总览

### 2.1 三层架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        前端层 (Vue 3)                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────────┐ │
│  │ 用户管理    │ │ 设备管理    │ │ 显示设置    │ │ 管理后台   │ │
│  │ - 注册/登录 │ │ - 绑定/解绑 │ │ - 卡片配置  │ │ - 用户管理 │ │
│  │ - 用户协议  │ │ - WiFi状态  │ │ - 天气设置  │ │ - OTA推送  │ │
│  │ - 修改密码  │ │ - 位置信息  │ │ - 字体设置  │ │ - 系统监控 │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └────────────┘ │
└──────────────────────────┬──────────────────────────────────────┘
                           │ HTTPS / WebSocket
┌──────────────────────────▼──────────────────────────────────────┐
│                     服务端层 (NestJS)                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────────┐ │
│  │ 用户服务    │ │ 设备服务    │ │ 天气服务    │ │ OTA服务    │ │
│  │ - JWT认证   │ │ - MQTT管理  │ │ - IP定位    │ │ - 固件管理 │ │
│  │ - 注册/登录 │ │ - 绑定逻辑  │ │ - 天气API   │ │ - 版本控制 │ │
│  │ - 权限控制  │ │ - 配置同步  │ │ - 数据缓存  │ │ - 差分升级 │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └────────────┘ │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ 数据库      │ │ MQTT Broker │ │ 外部API     │               │
│  │ - SQLite    │ │ - 消息路由  │ │ - 和风天气  │               │
│  │ - 用户表    │ │ - 设备发现  │ │ - IP定位    │               │
│  │ - 设备表    │ │ - 实时推送  │ │             │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ MQTT
┌──────────────────────────▼──────────────────────────────────────┐
│                     硬件层 (ESP-12F)                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────────┐ │
│  │ 连接管理    │ │ 显示引擎    │ │ 天气模块    │ │ 红外系统   │ │
│  │ - WiFi管理  │ │ - 前景渲染  │ │ - 天气解析  │ │ - 接收/发送│ │
│  │ - MQTT客户端│ │ - 背景渲染  │ │ - 自动切换  │ │ - 学习模式 │ │
│  │ - OTA升级   │ │ - 卡片管理  │ │             │ │ - 品牌协议 │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └────────────┘ │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ 输入系统    │ │ 配置管理    │ │ 传感器      │               │
│  │ - 按键处理  │ │ - EEPROM    │ │ - AHT20     │               │
│  │ - 中断模拟  │ │ - 配置同步  │ │ - NTP时钟   │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 数据流

#### 正常显示流程
```
ESP启动 → 连接WiFi → 连接MQTT → 上报IP
   ↓                                      ↓
等待服务器推送天气/配置 ← 服务器调用IP定位API
   ↓
渲染显示（前景+背景叠加）
   ↓
按键切换卡片/模式
```

#### 配置更新流程
```
用户前端操作 → 后端验证 → MQTT推送 → ESP接收 → EEPROM保存 → 实时生效
```

#### OTA升级流程
```
管理后台上传固件 → 选择目标设备 → MQTT推送OTA通知 → ESP下载 → 校验 → 重启
```

---

## 3. 硬件抽象层设计

### 3.1 IS31FL3733驱动抽象

#### 3.1.1 核心类设计

#### 3.1.2 硬件初始化流程



### 3.2 引脚定义（完整版）

```cpp
// config_pins.h
#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// I2C总线
#define PIN_SDA 4           // GPIO4 - I2C数据线
#define PIN_SCL 5           // GPIO5 - I2C时钟线
#define I2C_CLOCK_SPEED 400000

// IS31FL3733控制
#define PIN_SDB 0           // GPIO0 - Shutdown控制（低电平复位）

// 用户输入
#define PIN_BUTTON 16       // GPIO16 - 微动开关（高电平有效，内部下拉）

// 系统指示
#define PIN_LED_SYS 15      // GPIO15 - 系统状态LED

// 红外系统（复用现有）
#define PIN_IR_SEND 14      // GPIO14 - 红外发射（SS8050驱动）
#define PIN_IR_RECV 2       // GPIO2 - 红外接收（1838B）

// 传感器
#define PIN_AHT_SDA PIN_SDA // AHT20共享I2C总线
#define PIN_AHT_SCL PIN_SCL

// 预留扩展（3收1发）
// #define PIN_IR_RECV_2 12  // GPIO12 - 预留红外接收2
// #define PIN_IR_RECV_3 13  // GPIO13 - 预留红外接收3

// 屏幕参数
#define DISPLAY_WIDTH 42
#define DISPLAY_HEIGHT 11
#define NUM_CHIPS 6
#define CHIP_COLS 7

#endif
```

---

## 4. 引脚定义与连接

### 5.1 ESP-12F GPIO完整分配表

| GPIO | 功能 | 方向 | 电平 | 物理位置 | 说明 |
|:----:|------|:----:|:----:|:--------:|------|
| **IO0** | SDB | 输出 | 3.3V | 顶部(9脚) | IS31FL3733 Shutdown控制，**低电平复位** |
| **IO1** | TX | 输出 | 3.3V | 顶部(16脚) | 串口调试输出，烧录时可用 |
| **IO2** | IR_RECV_1 | 输入 | 3.3V | 底部 | 1838B红外接收器 **#1**（正面） |
| **IO3** | RX | 输入 | 3.3V | 顶部(15脚) | 串口输入，烧录时可用 |
| **IO4** | SDA | 双向 | 3.3V | 底部 | I2C数据线，**10kΩ上拉**，连接所有I2C设备 |
| **IO5** | SCL | 输出 | 3.3V | 底部 | I2C时钟线，**10kΩ上拉** |
| **IO12** | （预留） | 输入 | 3.3V | 顶部(14脚) | 预留IR_RECV_2（左侧面），暂缓开发 |
| **IO13** | （预留） | 输入 | 3.3V | 顶部(13脚) | 预留IR_RECV_3（右侧面），暂缓开发 |
| **IO14** | IR_SEND | 输出 | 3.3V | 顶部(4脚) | 红外发射驱动，接SS8050三极管 |
| **IO15** | LED_SYS | 输出 | 3.3V | 顶部(10脚) | 系统状态LED指示 |
| **IO16** | BUTTON | 输入 | 3.3V | 顶部(3脚) | 微动开关，**高电平有效**（按下去3.3V） |
| **ADC** | 悬空 | - | - | 顶部(2脚) | 未使用，悬空处理 |


### 5.2 I2C总线设备连接

**I2C总线拓扑**：
```
ESP-12F (GPIO4/5)
    │
    ├─ 4.7kΩ上拉电阻 ──┬─ 4.7kΩ上拉电阻 ── VCC(3.3V)
    │                  │
    SDA ───────────────┴──────────────────────┐
    SCL ──────────────────────────────────────┤
    │                                         │
    ├─ AHT20 (0x38) 温湿度传感器              │
    │                                         │
    ├─ IS31FL3733 #0 (0x50) 列0-6            │
    ├─ IS31FL3733 #1 (0x51) 列7-13           │
    ├─ IS31FL3733 #2 (0x52) 列14-20          │
    ├─ IS31FL3733 #3 (0x53) 列21-27          │
    ├─ IS31FL3733 #4 (0x54) 列28-34          │
    └─ IS31FL3733 #5 (0x55) 列35-41          │
                                             │
    SDB ─────────────────────────────────────┤
    (GPIO0控制所有芯片Shutdown)               │
```

**I2C地址配置详情**：

IS31FL3733通过ADDR1和ADDR2引脚连接不同电平设置地址：

| 芯片 | ADDR1引脚 | ADDR2引脚 | I2C地址 | 负责的列 | LED矩阵位置 |
|:----:|:---------:|:---------:|:-------:|:--------:|:-----------:|
| #0 | GND (0) | GND (0) | 0x50 | CS0-CS6 | 列0-6 |
| #1 | SCL (1) | GND (0) | 0x51 | CS0-CS6 | 列7-13 |
| #2 | SDA (2) | GND (0) | 0x52 | CS0-CS6 | 列14-20 |
| #3 | VCC (3) | GND (0) | 0x53 | CS0-CS6 | 列21-27 |
| #4 | GND (0) | SCL (1) | 0x54 | CS0-CS6 | 列28-34 |
| #5 | SCL (1) | SCL (1) | 0x55 | CS0-CS6 | 列35-41 |


### 5.3 LED矩阵物理布局

**总分辨率**：42列 × 11行 = 462个LED

```
芯片 #0      芯片 #1      芯片 #2      芯片 #3      芯片 #4      芯片 #5
(0x50)       (0x51)       (0x52)       (0x53)       (0x54)       (0x55)

```

### 5.4 红外发射驱动电路

### 5.5 红外接收电路

**当前实现（1收）**：
```
1838B红外接收器
    ┌─────────┐
  VCC(3.3V)──┤VCC  OUT├──► GPIO2 (ESP-12F)
    │        │     GND├──► GND
   100nF     └─────────┘
   电容
    │
   GND
```

**预留扩展（3收）**：
```
正面：    左侧面：    右侧面：
GPIO2     GPIO12      GPIO13
(1838B)   (预留)      (预留)
   │          │          │
   └──────────┴──────────┘
              │
            3.3V
```

### 5.6 按键电路

**高电平有效设计**：
```
        3.3V
          │
         [上拉电阻10kΩ]（内部下拉已在代码配置）
          │
    ┌─────┴─────┐
    │           │
  GPIO16     微动开关
(INPUT_      (6×6×5mm)
PULLDOWN)       │
    │           │
    └───────────┤
                │
               GND

按下去：GPIO16 = 3.3V（高电平）
松开：GPIO16 = 0V（低电平，内部下拉）
```

**说明**：ESP-12F的GPIO16有内部下拉电阻（INPUT_PULLDOWN_16），因此不需要外部下拉电阻。

### 5.7 系统电源连接


---

## 5. 红外系统（复用现有固件的d）

### 5.1 现有红外功能保留

完全复用现有的，不做任何修改（除非要集成）。

**功能列表**：
1. 红外发射（原始数据 + 品牌协议）
2. 红外接收（学习模式）
3. 自发自收过滤（1.5秒窗口）
4. 支持的品牌协议（IRremoteESP8266库支持的所有空调协议）

**接口保持不变**


### 5.2 扩展预留（3收1发）

虽然暂缓开发，但代码结构预留扩展：

```cpp


// 预留：360度红外接收
// - 3个1838B接收器分别位于前、左、右
// - 通过时间差判断信号方向
// - 用于空调Ghost检测优化

#endif
```

---

## 6. 显示引擎架构

### 5.1 核心概念定义

#### 5.1.1 前景（Foreground）
- **定义**：叠加在背景之上的信息显示
- **类型**：
  1. **时钟** - 显示当前时间（时:分:秒）
  2. **日期** - 显示年月日星期
  3. **温湿度** - AHT20传感器数据
- **属性**：
  - 字体大小（FONT_3x5 / FONT_5x7 / FONT_3x9 / FONT_5x5 / FONT_6x9）
  - 位置（自动根据背景调整，部分写死）
  - 亮度（0-255）
  - 自动切换间隔（秒）
  - 转场效果（硬切/淡入淡出/滑入滑出）

#### 5.1.2 背景（Background）
- **定义**：屏幕底层动态或静态图案
- **类型**：
  1. **矩阵雨** - 代码雨效果（大中小雨5秒自动切换）
  2. **水波纹** - 正弦波扩散（不可叠加前景）
  3. **生命游戏** - 细胞自动机（不可叠加前景）
  4. **沙漏** - 物理沙粒模拟（不可叠加前景）
  5. **Pong时钟** - 乒乓球游戏动画
  6. **天气系统**：
     - 火焰（30度及以上触发）
     - 下雨（大中小雨）
     - 刮风
     - 下雪
     - 晴天（太阳，后期加月亮）
     - 阴天
  7. **ABM呼吸灯** - 硬件自动呼吸

#### 5.1.3 卡片（Card）
- **定义**：用户创建的显示配置单元
- **结构**：1个卡片 = 0-1个背景 + 0-N个前景
- **属性**：
  - 卡片名称
  - 背景类型（可选）
  - 前景列表（每个前景独立设置）
  - 卡片切换转场效果
  - 是否启用自动切换
  - 自动切换间隔（秒）

### 5.2 类层次结构


### 5.3 背景-前景约束表（以后还会增加）

| 背景类型 | 允许前景 | 最大字体 | 位置限制 | 备注 |
|---------|---------|---------|---------|------|
| 矩阵雨 | ✅ | FONT_5x7 | 居中 | 自动清除前景区域 |
| 水波纹 | ❌ | - | - | 全屏动态，不支持叠加 |
| 生命游戏 | ❌ | - | - | 全屏动态，不支持叠加 |
| 沙漏 | ❌ | - | - | 全屏动态，不支持叠加 |
| Pong时钟 | ✅ | FONT_3x5 | 顶部 | 游戏区域固定 |
| 火焰(天气) | ✅ | **FONT_3x5** | **靠上写死** | 只支持小字体，不同字体位置不同 |
| 下雨(天气) | ✅ | FONT_5x7 | 居中 | - |
| 刮风(天气) | ✅ | FONT_5x7 | 居中 | - |
| 下雪(天气) | ✅ | FONT_5x7 | 居中 | - |
| 晴天(天气) | ✅ | FONT_5x7 | **靠右** | 避开左侧太阳 |
| 阴天(天气) | ✅ | FONT_5x7 | 居中 | - |
| ABM呼吸 | ✅ | FONT_5x7 | 居中 | 全屏均匀 |

### 5.4 具体渲染器实现

#### 5.4.1 时钟前景渲染器

参照test3733scanner.

#### 5.4.2 天气背景渲染器（以火焰为例）

参照test3733scanner.

---

## 7. 天气系统设计

### 6.1 系统架构（方案 - 混合方案）

```
ESP8266启动
    ↓
连接WiFi
    ↓
连接MQTT
    ↓
上报IP地址 ──MQTT──→ 服务器
                          ↓
                    调用IP定位API（如ip-api.com）
                          ↓
                    获取城市/坐标
                          ↓
                    存储到设备数据库
                          ↓
                    调用和风天气API（用城市/坐标）
                          ↓
                    缓存天气数据（10分钟）
                          ↓
         ←────MQTT推送────┘
    ↓
ESP解析天气数据
    ↓
根据温度触发火焰（≥30°C）
    ↓
渲染显示
```

### 6.2 服务器端天气服务
服务器梅格15分钟请求一次（根据在线的esp所在城市数量决定请求几个。同一城市只请求一次）和风天气试试天气api,存下来，esp请求的时候发回


### 6.3 天气匹配逻辑

esp开机时候先请求一个位置api（先请求外网ip,第三方api,）再请求ip-》位置api（第三方api） 然后把位置发给后端。然后esp向后端请求天气

### 6.4 前端天气设置

默认利用esp发回的位置 但是保留用户自己指定的功能

---

## 8. 转场效果系统

### 7.1 前景切换转场



### 7.2 卡片切换转场

卡片切换转场在前景转场基础上，增加了背景的整体切换：


---

## 9. 按键交互重构

### 8.1 问题分析

**当前问题**：
- IO16不支持中断（硬件限制）
- 现有代码（test3733scanner）全是补丁，难以维护
- 双击、长按检测逻辑混乱

### 8.2 新设计方案

使用EasyButton库，配合轮询+状态机：


### 8.3 按键功能映射

| 操作 | 触发条件 | 功能 |
|-----|---------|------|
| **短按** | 按下<500ms | 亮屏/息屏切换 |
| **长按** | 按下>900ms | 进入亮度调节模式（5秒超时） |
| **长按中短按** | 亮度模式下短按 | 切换亮度级别（20→60→120→200→255→20循环） |
| **双击** | 两次短按<400ms间隔 | 切换到下一张卡片 |

---

## 10. 字体系统

### 9.1 字体数据结构



### 9.2 字体设置器（预留实现）



---

## 11. OTA升级系统

### 10.1 服务端OTA服务


### 10.2 硬件端OTA处理



---

## 12. 后端服务改进

### 11.1 设备绑定健壮性改进

现有问题分析：
- 设备重复绑定逻辑不完善
- 防意外掉绑定机制缺失



### 11.2 用户系统扩展

```typescript
// auth.service.ts

@Injectable()
export class AuthService {
  // 注册（带用户协议）
  async register(dto: RegisterDto): Promise<User> {
    // 检查用户名是否已存在
    const existing = await this.usersRepository.findOne({
      where: [{ username: dto.username }, { email: dto.email }]
    });
    
    if (existing) {
      throw new ConflictException('用户名或邮箱已存在');
    }
    
    // 验证用户协议
    if (!dto.agreedToTerms) {
      throw new BadRequestException('必须同意用户协议才能注册');
    }
    
    const hashedPassword = await bcrypt.hash(dto.password, 10);
    
    const user = this.usersRepository.create({
      username: dto.username,
      email: dto.email,
      password: hashedPassword,
      agreedToTerms: true,
      termsAgreedAt: new Date(),
      createdAt: new Date(),
    });
    
    return await this.usersRepository.save(user);
  }
  
  // 修改密码
  async changePassword(userId: number, dto: ChangePasswordDto) {
    const user = await this.usersRepository.findOne({ where: { id: userId } });
    
    if (!user) {
      throw new NotFoundException('用户不存在');
    }
    
    // 验证旧密码
    const isMatch = await bcrypt.compare(dto.oldPassword, user.password);
    if (!isMatch) {
      throw new UnauthorizedException('旧密码错误');
    }
    
    // 更新密码
    user.password = await bcrypt.hash(dto.newPassword, 10);
    user.passwordChangedAt = new Date();
    
    await this.usersRepository.save(user);
    
    // 使所有现有token失效（强制重新登录）
    await this.revokeAllUserTokens(userId);
    
    return { message: '密码修改成功，请使用新密码重新登录' };
  }
}
```

### 11.3 管理后台API

```typescript
// admin.controller.ts

@Controller('admin')
@UseGuards(JwtAuthGuard, AdminGuard)  // 需要管理员权限
export class AdminController {
  constructor(
    private adminService: AdminService,
  ) {}
  
  // 获取所有用户
  @Get('users')
  async getAllUsers(@Query() query: QueryUsersDto) {
    return this.adminService.getAllUsers(query);
  }
  
  // 获取用户详情
  @Get('users/:id')
  async getUserDetail(@Param('id') userId: number) {
    return this.adminService.getUserDetail(userId);
  }
  
  // 获取所有设备
  @Get('devices')
  async getAllDevices(@Query() query: QueryDevicesDto) {
    return this.adminService.getAllDevices(query);
  }
  
  // 获取设备详情
  @Get('devices/:uuid')
  async getDeviceDetail(@Param('uuid') uuid: string) {
    return this.adminService.getDeviceDetail(uuid);
  }
  
  // 解绑设备
  @Post('devices/:uuid/unbind')
  async unbindDevice(@Param('uuid') uuid: string) {
    return this.adminService.unbindDevice(uuid);
  }
  
  // 推送OTA
  @Post('ota/push')
  async pushOta(@Body() dto: PushOtaDto) {
    return this.otaService.pushOtaBatch(dto.deviceIds, dto.version);
  }
  
  // 获取系统统计
  @Get('stats')
  async getSystemStats() {
    return this.adminService.getSystemStats();
  }
}
```

---

## 13. 前端界面设计

### 12.1 页面结构

```
前端路由：
├── /login              # 登录
├── /register           # 注册（带用户协议）
├── /forgot-password    # 忘记密码
├── /reset-password     # 重置密码
├── /                   # 首页（控制面板）
│   ├── /devices        # 设备管理
│   ├── /settings       # 显示设置（核心）
│   └── /profile        # 个人设置
└── /admin              # 管理后台（管理员）
    ├── /users          # 用户管理
    ├── /devices        # 设备管理
    └── /ota            # OTA管理
```

### 12.2 显示设置页面（核心）

```vue
<!-- DisplaySettings.vue -->
<template>
  <div class="display-settings">
    <!-- 卡片列表 -->
    <van-cell-group title="显示卡片">
      <van-swipe-cell v-for="(card, index) in cards" :key="card.id">
        <van-cell :title="card.name" 
                  :label="getCardLabel(card)"
                  @click="editCard(card)" />
        <template #right>
          <van-button square type="danger" text="删除" @click="deleteCard(index)" />
        </template>
      </van-swipe-cell>
      
      <van-cell title="添加卡片" icon="plus" @click="addCard" />
    </van-cell-group>
    
    <!-- 全局设置 -->
    <van-cell-group title="全局设置">
      <van-cell title="自动切换卡片">
        <template #right-icon>
          <van-switch v-model="globalSettings.autoSwitch" />
        </template>
      </van-cell>
      
      <van-cell v-if="globalSettings.autoSwitch"
                title="切换间隔"
                :value="globalSettings.switchInterval + '秒'"
                @click="showIntervalPicker = true" />
      
      <van-cell title="卡片转场效果"
                :value="getTransitionName(globalSettings.cardTransition)"
                @click="showCardTransitionPicker = true" />
    </van-cell-group>
    
    <!-- 天气设置 -->
    <weather-settings v-model="weatherSettings" />
    
    <!-- 卡片编辑弹窗 -->
    <card-editor v-model:show="showCardEditor"
                 :card="editingCard"
                 @save="saveCard" />
  </div>
</template>
```

### 12.3 卡片编辑器组件

```vue
<!-- CardEditor.vue -->
<template>
  <van-popup v-model:show="show" position="bottom" :style="{ height: '80%' }">
    <div class="card-editor">
      <van-nav-bar :title="isEdit ? '编辑卡片' : '新建卡片'" 
                   left-text="取消" 
                   right-text="保存"
                   @click-left="close"
                   @click-right="save" />
      
      <van-field v-model="card.name" label="卡片名称" placeholder="请输入名称" />
      
      <!-- 背景选择 -->
      <van-cell-group title="背景">
        <van-cell title="背景类型" 
                  :value="card.background?.type || '无'"
                  @click="showBackgroundPicker = true" />
        
        <van-cell v-if="card.background" title="背景亮度">
          <template #right-icon>
            <van-slider v-model="card.background.brightness" :min="0" :max="255" />
          </template>
        </van-cell>
      </van-cell-group>
      
      <!-- 前景列表 -->
      <van-cell-group title="前景（可多个）">
        <van-cell v-for="(fg, index) in card.foregrounds" :key="fg.id"
                  :title="getForegroundName(fg.type)"
                  :label="getForegroundLabel(fg)"
                  @click="editForeground(index)">
          <template #right-icon>
            <van-icon name="cross" @click.stop="removeForeground(index)" />
          </template>
        </van-cell>
        
        <van-cell title="添加前景" icon="plus" @click="addForeground" />
      </van-cell-group>
      
      <!-- 前景编辑弹窗 -->
      <foreground-editor v-model:show="showForegroundEditor"
                        :foreground="editingForeground"
                        :background="card.background"
                        @save="saveForeground" />
    </div>
  </van-popup>
</template>
```

### 12.4 前景编辑器

```vue
<!-- ForegroundEditor.vue -->
<template>
  <van-popup v-model:show="show" position="right" :style="{ width: '100%', height: '100%' }">
    <van-nav-bar :title="'编辑前景'" left-arrow @click-left="close" />
    
    <van-cell-group>
      <!-- 前景类型 -->
      <van-cell title="类型" 
                :value="foreground.type"
                @click="showTypePicker = true" />
      
      <!-- 字体大小 -->
      <van-cell title="字体大小"
                :value="getFontName(foreground.font)"
                @click="showFontPicker = true" />
      
      <!-- 亮度 -->
      <van-cell title="亮度">
        <template #right-icon>
          <van-slider v-model="foreground.brightness" :min="0" :max="255" />
        </template>
      </van-cell>
      
      <!-- 自动切换 -->
      <van-cell title="自动切换">
        <template #right-icon>
          <van-switch v-model="foreground.autoSwitch" />
        </template>
      </van-cell>
      
      <van-cell v-if="foreground.autoSwitch"
                title="切换间隔（秒）"
                :value="foreground.autoSwitchInterval + '秒'"
                @click="showIntervalPicker = true" />
      
      <!-- 转场效果 -->
      <van-cell title="转场效果"
                :value="getTransitionName(foreground.transition)"
                @click="showTransitionPicker = true" />
    </van-cell-group>
    
    <!-- 预览 -->
    <div class="preview">
      <led-simulator :card="previewCard" />
    </div>
  </van-popup>
</template>
```

### 12.5 管理后台界面

```vue
<!-- AdminDashboard.vue -->
<template>
  <div class="admin-dashboard">
    <van-nav-bar title="管理后台" />
    
    <!-- 统计卡片 -->
    <van-grid :column-num="2">
      <van-grid-item icon="user-o" :text="`用户: ${stats.userCount}`" />
      <van-grid-item icon="desktop-o" :text="`设备: ${stats.deviceCount}`" />
      <van-grid-item icon="online" :text="`在线: ${stats.onlineCount}`" />
      <van-grid-item icon="warning-o" :text="`离线7天: ${stats.offlineCount}`" />
    </van-grid>
    
    <!-- 功能入口 -->
    <van-cell-group>
      <van-cell title="用户管理" is-link to="/admin/users" />
      <van-cell title="设备管理" is-link to="/admin/devices" />
      <van-cell title="OTA管理" is-link to="/admin/ota" />
      <van-cell title="系统日志" is-link to="/admin/logs" />
    </van-cell-group>
  </div>
</template>
```

---

## 14. API接口规范

### 13.1 设备相关接口

```yaml
# 设备绑定
POST /api/devices
Request:
  uuid: string        # 设备UUID
  name: string        # 设备名称
  mac: string         # MAC地址
  ip: string          # IP地址
Response:
  id: number
  uuid: string
  userId: number
  name: string
  status: string      # online/offline

# 获取设备列表
GET /api/devices
Response:
  devices: [
    {
      id: number
      uuid: string
      name: string
      status: string
      wifiName: string
      wifiSignal: number
      location: string    # 城市
      lastSeenAt: datetime
      currentCard: string # 当前卡片ID
    }
  ]

# 发送命令
POST /api/devices/{id}/cmd
Request:
  type: string        # switch_card / update_config / ota
  payload: object

# 获取设备日志
GET /api/devices/{id}/logs?limit=50&action=
Response:
  logs: [...]

# 解绑设备
DELETE /api/devices/{id}
```

### 13.2 显示设置接口

```yaml
# 获取显示配置
GET /api/devices/{id}/display
Response:
  cards: [
    {
      id: string
      name: string
      background: {
        type: string
        brightness: number
      }
      foregrounds: [
        {
          id: string
          type: string
          font: string
          brightness: number
          autoSwitchInterval: number
          transition: string
        }
      ]
      cardTransition: string
      autoSwitch: boolean
      autoSwitchInterval: number
    }
  ]
  globalSettings: {
    autoSwitch: boolean
    switchInterval: number
    cardTransition: string
  }
  weatherSettings: {
    enabled: boolean
    locationMode: 'auto' | 'manual'
    manualCity: string
  }

# 更新显示配置
PUT /api/devices/{id}/display
Request:
  cards: [...]
  globalSettings: {...}
  weatherSettings: {...}

# 切换卡片（立即生效）
POST /api/devices/{id}/switch-card
Request:
  cardId: string
```

### 13.3 天气接口

```yaml
# 获取设备位置
GET /api/devices/{id}/location
Response:
  mode: 'auto' | 'manual'
  city: string
  coordinates: { lat: number, lon: number }
  ipAddress: string

# 更新天气位置（手动模式）
PUT /api/devices/{id}/location
Request:
  mode: 'manual'
  city: string

# 获取当前天气
GET /api/devices/{id}/weather
Response:
  temperature: number
  humidity: number
  weatherCode: string
  weatherName: string
  windSpeed: number
  windDir: number
  displayTypes: string[]  # 需要显示的背景类型
  isDay: boolean
```

### 13.4 OTA接口

```yaml
# 上传固件
POST /api/ota/upload
Content-Type: multipart/form-data
Body:
  firmware: File
  version: string
  releaseNotes: string

# 获取固件版本列表
GET /api/ota/versions
Response:
  versions: [
    {
      version: string
      fileName: string
      fileSize: number
      checksum: string
      releaseNotes: string
      createdAt: datetime
    }
  ]

# 推送OTA
POST /api/ota/push
Request:
  deviceIds: string[]
  version: string

# 批量推送
POST /api/ota/push-batch
Request:
  deviceIds: string[]
  version: string
```

### 13.5 管理后台接口

```yaml
# 获取所有用户
GET /api/admin/users?page=1&limit=20
Response:
  users: [
    {
      id: number
      username: string
      email: string
      deviceCount: number
      createdAt: datetime
      lastLoginAt: datetime
    }
  ]
  total: number

# 获取用户详情
GET /api/admin/users/{id}
Response:
  user: {...}
  devices: [...]

# 获取所有设备
GET /api/admin/devices?page=1&limit=20&status=
Response:
  devices: [
    {
      id: number
      uuid: string
      name: string
      userId: number
      username: string
      wifiName: string
      wifiSignal: number
      location: string
      ipAddress: string
      status: string
      lastSeenAt: datetime
      bindAt: datetime
    }
  ]

# 系统统计
GET /api/admin/stats
Response:
  userCount: number
  deviceCount: number
  onlineCount: number
  offlineCount: number
  todayNewUsers: number
  todayNewDevices: number
```

---

## 15. 数据模型

### 14.1 数据库Schema

```sql
-- 用户表
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  username VARCHAR(50) UNIQUE NOT NULL,
  email VARCHAR(100) UNIQUE NOT NULL,
  password VARCHAR(255) NOT NULL,
  isAdmin BOOLEAN DEFAULT 0,
  agreedToTerms BOOLEAN DEFAULT 0,
  termsAgreedAt DATETIME,
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP,
  updatedAt DATETIME DEFAULT CURRENT_TIMESTAMP,
  lastLoginAt DATETIME,
  passwordChangedAt DATETIME
);

-- 设备表
CREATE TABLE devices (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  uuid VARCHAR(32) UNIQUE NOT NULL,
  userId INTEGER REFERENCES users(id),
  name VARCHAR(100),
  macAddress VARCHAR(17),
  ipAddress VARCHAR(15),
  wifiName VARCHAR(100),
  wifiSignal INTEGER,
  location VARCHAR(100),  -- 城市
  coordinates VARCHAR(50), -- "lat,lon"
  
  -- 显示配置（JSON存储）
  displayConfig TEXT,  -- { cards: [...], globalSettings: {...}, weatherSettings: {...} }
  
  -- 状态
  status VARCHAR(20) DEFAULT 'offline',  -- online/offline
  lastSeenAt DATETIME,
  bindAt DATETIME,
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP,
  updatedAt DATETIME DEFAULT CURRENT_TIMESTAMP,
  
  -- OTA
  currentVersion VARCHAR(20),
  targetVersion VARCHAR(20),
  otaStatus VARCHAR(20)  -- idle/downloading/installing/rebooting
);

-- 操作日志表
CREATE TABLE audit_logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  deviceId INTEGER REFERENCES devices(id),
  userId INTEGER REFERENCES users(id),
  action VARCHAR(50) NOT NULL,  -- bind/unbind/command/update_config/ota
  payload TEXT,
  ipAddress VARCHAR(15),
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 传感器数据表
CREATE TABLE sensor_readings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  deviceId INTEGER REFERENCES devices(id),
  temperature DECIMAL(4,1),
  humidity DECIMAL(4,1),
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 固件版本表
CREATE TABLE firmware_versions (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  version VARCHAR(20) UNIQUE NOT NULL,
  fileName VARCHAR(255) NOT NULL,
  fileSize INTEGER,
  checksum VARCHAR(32),
  releaseNotes TEXT,
  downloadUrl VARCHAR(500),
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 用户协议版本表
CREATE TABLE terms_versions (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  version VARCHAR(10) NOT NULL,
  content TEXT NOT NULL,
  effectiveAt DATETIME NOT NULL,
  createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

---


## 附录

### A. 硬件引脚图

```
ESP-12F引脚分配：

GPIO0   → SDB (IS31FL3733 Shutdown)
GPIO1   → TX (Debug)
GPIO2   → IR_RECV (1838B)
GPIO3   → RX (Debug)
GPIO4   → SDA (I2C) → AHT20 + 6×IS31FL3733
GPIO5   → SCL (I2C)
GPIO12  → （预留IR_RECV_2）
GPIO13  → （预留IR_RECV_3）
GPIO14  → IR_SEND (SS8050驱动4个LED)
GPIO15  → LED_SYS
GPIO16  → BUTTON (微动开关，高电平有效)

I2C地址：
- AHT20: 0x38
- IS31FL3733 #0: 0x50
- IS31FL3733 #1: 0x51
- IS31FL3733 #2: 0x52
- IS31FL3733 #3: 0x53
- IS31FL3733 #4: 0x54
- IS31FL3733 #5: 0x55
```

### B. 字体规格表

| 字体 | 宽度 | 高度 | 用途 |
|-----|------|------|------|
| FONT_3x5 | 3px | 5px | 火焰背景专用 |
| FONT_5x7 | 5px | 7px | 标准显示 |
| FONT_3x9 | 3px | 9px | 高窄数字 |
| FONT_5x5 | 5px | 5px | 中文显示 |
| FONT_6x9 | 6px | 9px | 大数字时钟 |

### C. 背景-前景约束速查表

| 背景 | 可叠加 | 最大字体 | 位置 |
|-----|--------|---------|------|
| 矩阵雨 | ✅ | 5x7 | 居中 |
| 水波纹 | ❌ | - | - |
| 生命游戏 | ❌ | - | - |
| 沙漏 | ❌ | - | - |
| Pong | ✅ | 3x5 | 顶部 |
| 火焰 | ✅ | **3x5** | **靠上** |
| 下雨 | ✅ | 5x7 | 居中 |
| 刮风 | ✅ | 5x7 | 居中 |
| 下雪 | ✅ | 5x7 | 居中 |
| 晴天 | ✅ | 5x7 | **靠右** |
| 阴天 | ✅ | 5x7 | 居中 |

---


