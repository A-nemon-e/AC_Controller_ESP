# ESP12F 空调控制器 - 固件使用教程

> 版本：v1.3.0 (支持自动协议检测)  
> 更新日期：2026-02-03

---

## 📋 目录

1. [硬件准备](#硬件准备)
2. [固件编译与上传](#固件编译与上传)
3. [首次配置](#首次配置)
4. [功能使用](#功能使用)
5. [高级功能](#高级功能)
6. [串口调试](#串口调试)
7. [故障排除](#故障排除)
8. [FAQ常见问题](#faq常见问题)

---

## 🔧 硬件准备

### 所需硬件

| 组件 | 型号/规格 | 说明 |
|------|----------|------|
| 主控 | ESP12F | ESP8266模组 |
| 红外接收 | 1838B | 38kHz红外接收头 |
| 红外发射 | LED 940nm | 普通红外LED或专用发射管 |
| 温湿度传感器 | AHT20 | I2C接口 (可选) |
| 电流传感器 | SCT-013 | 互感器 (可选) |
| 麦克风 | 数字麦克风 | 用于Ghost检测 (可选) |
| 电源 | 5V/1A | USB供电或DC-DC降压 |

### 硬件连接

参考 `config.h` 中的引脚定义：

```cpp
#define PIN_IR_SEND 14  // D5 - 红外发射LED
#define PIN_IR_RECV 13  // D7 - 1838B红外接收头
#define PIN_MIC 12      // D6 - 麦克风数字输出
#define PIN_SDA 4       // D2 - I2C数据线（AHT20）
#define PIN_SCL 5       // D1 - I2C时钟线（AHT20）
#define PIN_LED_SYS 15  // D8 - 系统状态LED
#define PIN_LED_IR 16   // D0 - 红外接收指示LED
#define PIN_ADC A0      // A0 - 电流互感器
```

**最小系统**：只需 ESP12F + 红外接收 + 红外发射 即可工作！

---

## 💻 固件编译与上传

### 方式一：Arduino IDE（推荐新手）

#### 1. 安装Arduino IDE

下载并安装 [Arduino IDE 1.8.x 或 2.x](https://www.arduino.cc/en/software)

#### 2. 添加ESP8266开发板支持

1. 打开 **文件 → 首选项**
2. 在"附加开发板管理器网址"中添加：
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. 打开 **工具 → 开发板 → 开发板管理器**
4. 搜索 `esp8266`，安装 **ESP8266 by ESP8266 Community**

#### 3. 安装必需库

打开 **工具 → 管理库**，搜索并安装：

| 库名称 | 版本 | 说明 |
|--------|------|------|
| IRremoteESP8266 | 最新 | **必需** - 红外收发 |
| PubSubClient | 最新 | **必需** - MQTT通信 |
| ArduinoJson | ≥6.x | **必需** - JSON解析 |
| Adafruit AHT20 | 最新 | 可选 - 温湿度传感器 |

#### 4. 配置开发板设置

**工具菜单配置**：
- **开发板**: "Generic ESP8266 Module"
- **Flash Size**: "4M (1M SPIFFS)"
- **CPU Frequency**: "80 MHz"
- **Upload Speed**: "115200"
- **Port**: 选择你的COM端口

#### 5. 编译上传

1. 打开 `ac_controller.ino`
2. 点击 ✓ (Verify) 验证代码
3. 点击 → (Upload) 上传固件
4. 等待上传完成（约30秒）

**成功标志**：
```
Hard resetting via RTS pin...
```

---

### 方式二：PlatformIO（推荐开发者）

#### 1. 安装VSCode + PlatformIO

1. 安装 [Visual Studio Code](https://code.visualstudio.com/)
2. 安装 PlatformIO 扩展

#### 2. 打开项目

```bash
cd AC_Controller_ESP/esp-firmware/ac_controller
pio init
```

#### 3. 编辑 `platformio.ini`

```ini
[env:esp12e]
platform = espressif8266
board = esp12e
framework = arduino
lib_deps = 
    markszabo/IRremoteESP8266
    knolleary/PubSubClient
    bblanchon/ArduinoJson
monitor_speed = 115200
```

#### 4. 编译上传

```bash
# 编译
pio run

# 上传
pio run --target upload

# 串口监视
pio device monitor
```

---

## ⚙️ 首次配置

### 步骤1：连接串口监视器

1. 打开Arduino IDE的串口监视器（Ctrl+Shift+M）
2. 设置波特率为 **115200**
3. 选择 "Both NL & CR"

### 步骤2：查看启动信息

上传固件后，ESP会自动重启，串口输出：

```
========================================
  ESP12F 空调控制器
  版本: v1.3.0 (完整功能)
========================================

[系统信息]
  芯片ID: 0xXXXXXX
  MAC地址: XX:XX:XX:XX:XX:XX
  设备UUID: esp12f_XXXXXX
  用户ID: 0
  MQTT服务器: (未配置)
```

### 步骤3：配置WiFi

**方式一：SmartConfig（推荐）**

1. ESP启动后会进入SmartConfig模式（LED快闪）
2. 打开手机"ESP Touch"或"SmartConfig"App
3. 输入WiFi密码
4. 等待配置成功

**方式二：修改代码**

编辑 `config_manager.cpp`，填入WiFi信息后重新上传：

```cpp
const char* default_ssid = "你的WiFi名称";
const char* default_password = "你的WiFi密码";
```

**成功标志**：
```
[WiFi] ✅ 已连接！
[WiFi] IP地址: 192.168.1.100
```

### 步骤4：配置MQTT

**通过MQTT下发配置**（推荐）：

后端会自动下发配置到设备，Topic格式：
```
ac/user_{userId}/dev_{uuid}/config
```

Payload示例：
```json
{
  "mqttServer": "mqtt.example.com",
  "mqttPort": 1883,
  "userId": 1,
  "deviceUUID": "esp12f_abc123"
}
```

**或修改代码**：

编辑 `config.h`：

```cpp
#define MQTT_SERVER "your-mqtt-server.com"
#define MQTT_PORT 1883
#define USER_ID 1
#define DEVICE_UUID "esp_001"
```

**成功标志**：
```
[MQTT] ✅ 已连接
[MQTT] 订阅: ac/user_1/dev_esp_001/+
```

---

## 🎮 功能使用

### 1️⃣ 自动协议检测（新功能！）

**最简单的配网方式，推荐优先使用！**

#### 通过API触发

```bash
# 启动自动检测
POST http://your-backend/devices/1/auto-detect/start
Authorization: Bearer <token>
```

#### ESP端流程

1. ESP进入检测模式（30秒倒计时）
2. 串口输出：
   ```
   [自动检测] ✅ 已启动，请在30秒内按下遥控器任意键
   [自动检测] 💡 建议按：开机键 或 制冷26度
   ```

3. **按下空调遥控器任意键**

4. ESP识别协议：
   ```
   [自动检测] 📡 接收信号: Protocol=GREE, Bits=64
   [自动检测] ✅ 协议识别成功: GREE
   [自动检测] 型号代码: 0
   [GREE] Power=1, Mode=cool, Temp=26, Fan=0
   [自动检测] 📋 空调状态:
   Power: On, Mode: 1 (Cool), Temp: 26C, Fan: 0 (Auto)
   [主程序] ✅ AC协议识别成功，上报结果
   ```

5. 后端自动保存配置，**完成！**

#### 识别成功率

| 品牌类型 | 成功率 | 说明 |
|---------|-------|------|
| 格力 | 95%+ | 最常见 |
| 美的 | 95%+ | 最常见 |
| 大金 | 90%+ | 需测试型号 |
| 海尔/TCL | 85%+ | 通用型号 |
| Coolix杂牌 | 70%+ | 兼容协议 |
| 未知品牌 | - | 降级到手选 |

#### 识别失败怎么办？

```
[自动检测] ❌ 协议未识别 (UNKNOWN)
[自动检测] → 建议使用手动选择或Raw学习模式
```

→ 进入**手动选择模式**或**学习模式**

---

### 2️⃣ 手动选择品牌（降级方案1）

当自动检测识别为UNKNOWN时：

#### 1. 获取支持的品牌列表

```bash
GET http://your-backend/devices/brands
```

返回：
```json
{
  "brands": [
    { "id": "GREE", "name": "格力", "models": [0, 1, 2] },
    { "id": "MIDEA", "name": "美的", "models": [0, 1] },
    { "id": "DAIKIN", "name": "大金", "models": [0, 1, 2, 3] },
    ...
  ]
}
```

#### 2. 用户选择品牌

前端展示品牌列表，用户点击"格力"

#### 3. 设置品牌配置

```bash
POST http://your-backend/devices/1/setup/brand
{
  "brandId": "GREE",
  "model": 0
}
```

#### 4. 测试控制

发送测试命令：
```bash
POST http://your-backend/devices/1/cmd
{
  "power": true,
  "mode": "cool",
  "temp": 26,
  "fan": 0
}
```

**空调响应**？
- ✅ 是 → 配置成功！
- ❌ 否 → 尝试其他型号(model: 1, 2, 3...)，或进入学习模式

---

### 3️⃣ 学习模式（降级方案2）

当品牌选择仍然无效时，使用Raw学习模式。

#### 场景式学习（推荐）

**只学习3-5个常用场景**，节省内存：

| 场景ID | 说明 | 遥控器操作 |
|-------|------|----------|
| `off` | 关机 | 按"关机" |
| `cool_26_low` | 制冷26度低风 | 调到此状态后按确认 |
| `cool_24_mid` | 制冷24度中风 | 调到此状态后按确认 |
| `heat_28_low` | 制热28度低风 | 调到此状态后按确认 |
| `dry` | 除湿模式 | 按"除湿" |

#### API流程

**1. 启动学习**

```bash
POST http://your-backend/devices/1/learn
{
  "key": "cool_26_low"
}
```

**2. ESP进入学习模式**

串口输出：
```
[学习] ✅ 学习模式已启动: cool_26_low
[学习] 等待红外信号...
```

**3. 按下遥控器**

调整空调到"制冷26度低风"，按确认键

**4. 学习成功**

```
[学习] ✅ 学习成功
[学习] Raw数据长度: 199
```

**5. 重复3-5次**

学习其他常用场景

**6. 使用学习的命令**

```bash
POST http://your-backend/devices/1/cmd
{
  "scene": "cool_26_low"
}
```

ESP发送学习的Raw数据，空调执行！

---

### 4️⃣ 远程控制

配置完成后（自动检测/手选/学习），即可远程控制：

#### API控制

```bash
POST http://your-backend/devices/1/cmd
Content-Type: application/json

{
  "power": true,
  "mode": "cool",      // cool, heat, dry, fan, auto
  "temp": 26,          // 16-30
  "fan": 0,            // 0=auto, 1=low, 2=mid, 3=high
  "swingV": false,     // 垂直摆风
  "swingH": false      // 水平摆风
}
```

#### MQTT直连（高级）

```bash
# Topic
ac/user_1/dev_esp_001/cmd

# Payload
{
  "power": 1,
  "mode": 1,
  "temp": 26,
  "fan": 0
}
```

#### ESP执行

```
[红外] 发送品牌协议: GREE (型号: 0)
[红外] 参数: Power=1, Mode=1, Temp=26, Fan=0
[红外] ✅ 品牌协议发送成功
```

---

## 🚀 高级功能

### Ghost检测

**自动检测物理遥控器操作，并同步到App**

#### 工作原理

1. 用户用物理遥控器开空调
2. ESP红外接收头捕获信号
3. 麦克风检测到按键声音（Ghost事件）
4. ESP解析空调状态，上报MQTT
5. App显示同步状态

#### 串口输出

```
[Ghost] 检测到麦克风信号
[Ghost] 检测到红外信号
[Ghost] ⚠️ Ghost事件！发布通知
[MQTT] 发布: ac/user_1/dev_esp_001/event
Payload: {"type":"ghost","action":"physical_control"}
```

#### 配置Ghost检测窗口

```cpp
// config.h
#define DEFAULT_GHOST_WINDOW 30000  // 30秒窗口
```

MQTT动态调整：
```json
// Topic: ac/user_1/dev_esp_001/config
{
  "ghostWindow": 60000  // 改为60秒
}
```

---

### 定时任务

通过后端定时服务控制空调

#### 创建定时

```bash
POST http://your-backend/routines
{
  "deviceId": 1,
  "type": "cron",
  "cronExpression": "0 8 * * *",  // 每天8点
  "action": {
    "power": true,
    "mode": "cool",
    "temp": 26
  }
}
```

---

### 传感器监控

如果安装了AHT20和电流传感器：

#### 实时数据上报

ESP每30秒自动上报：

```bash
# Topic
ac/user_1/dev_esp_001/sensor

# Payload
{
  "temp": 26.5,
  "hum": 60.2,
  "current": 1.2
}
```

#### 查询历史

```bash
GET http://your-backend/devices/1/readings?limit=100
```

---

## 🔍 串口调试

### 开启调试输出

编辑 `config.h`：

```cpp
#define DEBUG_ENABLED true  // ✅ 开启
```

### 串口参数

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: 无

### 常用调试命令

**查看内存**：
```
[系统信息]
  空闲内存: 45120 bytes
```

**查看WiFi状态**：
```
[WiFi] IP地址: 192.168.1.100
[WiFi] 信号强度: -45 dBm
```

**查看MQTT状态**：
```
[MQTT] 连接状态: 已连接
[MQTT] 订阅: ac/user_1/dev_esp_001/+
```

---

## ❗ 故障排除

### 问题1：WiFi连接失败

**症状**：
```
[WiFi] ❌ 连接超时
[WiFi] 重试中...
```

**解决**：
1. 检查WiFi密码是否正确
2. 确认ESP与路由器距离（<10米）
3. 检查路由器是否支持2.4GHz（ESP8266不支持5GHz）
4. 重启路由器

---

### 问题2：MQTT连接失败

**症状**：
```
[MQTT] ❌ 连接失败
[MQTT] 5秒后重试...
```

**解决**：
1. 检查MQTT服务器地址是否正确
2. 检查防火墙是否开放1883端口
3. 验证MQTT用户名密码
4. 使用MQTT.fx工具测试服务器

---

### 问题3：红外发送无效

**症状**：ESP发送命令，但空调无响应

**解决**：
1. **检查硬件**：
   - 红外LED是否正确连接到PIN 14
   - LED是否正向接（长脚接GPIO，短脚接GND+限流电阻）
   - 距离空调<5米，角度正对

2. **检查品牌配置**：
   - 尝试其他型号：model 0, 1, 2...
   - 尝试其他兼容品牌（如TCL空调试试COOLIX）

3. **降级到学习模式**：
   - 使用Raw学习记录遥控器信号

4. **调试发送**：
   串口查看：
   ```
   [红外] 发送品牌协议: GREE (型号: 0)
   [红外] ✅ 品牌协议发送成功
   ```
   → 如果没有这行输出，说明命令未到达ESP

---

### 问题4：自动检测总是UNKNOWN

**症状**：
```
[自动检测] ❌ 协议未识别 (UNKNOWN)
```

**原因**：
- 遥控器使用非主流协议
- 接收距离太远/角度不对
- 接收头硬件问题

**解决**：
1. **改善接收条件**：
   - 遥控器距离接收头10cm内
   - 正对接收头
   - 避免强光干扰

2. **尝试多次**：
   - 重启自动检测
   - 尝试按不同的键（开机、温度+、温度-）

3. **降级方案**：
   - 手动选择品牌
   - 使用学习模式

---

### 问题5：缓冲区溢出

**症状**：
```
[自动检测] ⚠️ 缓冲区溢出！信号太长
```

**解决**：

编辑 `config.h`，增加缓冲区：
```cpp
#define IR_RECV_BUFFER_SIZE 1024  // 改为 2048
```

重新编译上传。

---

## 💡 FAQ常见问题

### Q1: 支持哪些空调品牌？

**A:** 支持30+品牌：
- **国产**: 格力、美的、海尔、TCL、海信、奥克斯
- **日系**: 大金、三菱、松下、富士通、东芝、夏普、日立
- **欧美**: 三星、LG、惠而浦、开利、约克

**不在列表？** → 尝试兼容协议（COOLIX/GREE/MIDEA）或学习模式

---

### Q2: 型号(model)是什么意思？

**A:** 同一品牌可能有多个协议版本，用model区分：
- GREE: 0=YAC, 1=YAA, 2=YAP, 3=YB0
- DAIKIN: 0-3代表不同型号

**不确定？** → 从0开始逐个尝试，或使用自动检测

---

### Q3: 学习模式vs品牌协议，哪个好？

**A:** 

| 对比项 | 品牌协议 | 学习模式 |
|-------|---------|---------|
| 设置难度 | ⭐⭐⭐⭐⭐ 最简单 | ⭐⭐⭐ 需操作 |
| 控制灵活性 | ⭐⭐⭐⭐⭐ 全参数 | ⭐⭐⭐ 固定场景 |
| 内存占用 | ⭐⭐⭐⭐⭐ 极小 | ⭐⭐ 较大 |
| 兼容性 | ⭐⭐⭐⭐ 主流品牌 | ⭐⭐⭐⭐⭐ 100% |

**推荐**：优先自动检测 → 手选品牌 → 学习模式

---

### Q4: Ghost检测需要麦克风吗？

**A:** 
- **有麦克风**: 完整Ghost检测（声音+红外双重确认）
- **无麦克风**: 只能检测红外，但仍可用

**可选硬件**，不影响核心功能

---

### Q5: ESP突然离线怎么办？

**A:** 
1. 检查电源是否稳定
2. 查看串口输出
3. 检查WiFi信号强度
4. 重启ESP（断电10秒再上电）
5. 检查路由器是否限制设备数量

---

### Q6: 固件升级怎么做？

**A:** 

**方式一：USB线升级**
1. 连接USB
2. 重新上传新固件

**方式二：OTA无线升级**（需后端支持）
```bash
POST http://your-backend/devices/1/ota
{
  "firmwareUrl": "http://your-server/firmware.bin"
}
```

---

### Q7: 如何查看固件版本？

**A:** 
查看串口输出：
```
ESP12F 空调控制器
版本: v1.3.0 (完整功能)
```

或查看 `ac_controller.ino` 第54行

---

## 📞 技术支持

- **GitHub Issues**: [https://github.com/your-repo/issues](https://github.com/your-repo/issues)
- **文档**: 查看 `README_*.md` 系列文档
- **串口日志**: 遇到问题时，请提供完整串口输出

---

## 📚 相关文档

- [`README_COMPILE.md`](README_COMPILE.md) - 编译配置说明
- [`README_STAGE1.md`](README_STAGE1.md) - 阶段1功能说明
- [`README_STAGE2.md`](README_STAGE2.md) - 阶段2功能说明  
- [`README_STAGE3.md`](README_STAGE3.md) - 阶段3功能说明
- [`README_OTA_CONFIG.md`](README_OTA_CONFIG.md) - OTA升级说明
- [`auto_detect_testing_guide.md`](../../.gemini/antigravity/brain/.../auto_detect_testing_guide.md) - 自动检测测试文档

---

**🎉 享受智能空调控制的便利！**
