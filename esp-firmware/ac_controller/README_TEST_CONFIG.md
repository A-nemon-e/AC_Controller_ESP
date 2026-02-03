# ESP固件 - 测试环境快速配置指南

> **适用场景**：没有App、没有前端，仅测试ESP固件和后端通信

---

## ❗ 核心问题解答

### Q1: 没有App和前端，怎么配置WiFi？

**A:** 有3种方法，推荐方法1（最简单）

### Q2: 后端地址在哪里配置？

**A:** 在 `config.h` 文件中硬编码

### Q3: SmartConfig为什么LED不闪？

**A:** `led_indicator.cpp` 中的 `STATUS_UNCONFIGURED` 可能设置为常亮/常灭，我们稍后修复

---

## 🚀 方法1：直接硬编码WiFi和MQTT（最简单）

### 步骤1：修改 `config.h`

打开 `config.h`，找到第23-42行，修改为你的实际值：

```cpp
// ===== WiFi配置 =====
// ⚠️ 测试环境：直接硬编码WiFi信息
#define WIFI_SSID "你的WiFi名称"           // ✅ 改这里
#define WIFI_PASSWORD "你的WiFi密码"       // ✅ 改这里

#define WIFI_CONNECT_TIMEOUT 20000
#define WIFI_RECONNECT_DELAY 5000

// ===== MQTT配置 =====
#define MQTT_SERVER "192.168.1.100"        // ✅ 改这里：你的后端服务器IP
#define MQTT_PORT 1883                     // ✅ 如果改了MQTT端口，改这里
#define MQTT_USER "esp_device"             // ✅ MQTT用户名
#define MQTT_PASSWORD "your_password"      // ✅ MQTT密码

// 设备标识
#define DEVICE_UUID "test_esp_001"         // ✅ 测试用UUID
#define USER_ID 1                          // ✅ 测试用户ID（对应后端的userId）
```

### 步骤2：修改 `wifi_manager.cpp`

让WiFi直接使用硬编码的凭证，不走SmartConfig。

找到 `WiFiManager::connect()` 函数（第11行），修改为：

```cpp
void WiFiManager::connect() {
  DEBUG_PRINTLN("\n[WiFi] 初始化WiFi模块");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  // ⚠️ 测试环境：直接使用硬编码的WiFi凭证
  #ifdef WIFI_SSID
    DEBUG_PRINTLN("[WiFi] 使用硬编码WiFi凭证");
    configured = attemptConnection(WIFI_SSID, WIFI_PASSWORD);
    return;
  #endif

  // 尝试从EEPROM读取凭证
  String ssid, password;
  if (loadCredentials(ssid, password)) {
    DEBUG_PRINTLN("[WiFi] 找到保存的凭证，尝试连接...");
    configured = attemptConnection(ssid, password);
  }

  // 如果没有凭证或连接失败，启动SmartConfig
  if (!configured) {
    DEBUG_PRINTLN("[WiFi] 未配置或连接失败，启动SmartConfig");
    startSmartConfig();
  }
}
```

### 步骤3：编译上传

1. Arduino IDE中打开 `ac_controller.ino`
2. 点击 ✓ 验证
3. 点击 → 上传
4. 等待上传完成

### 步骤4：查看串口输出

串口监视器（115200波特率）应该显示：

```
========================================
  ESP12F 空调控制器
  版本: v1.3.0 (完整功能)
========================================

[WiFi] 初始化WiFi模块
[WiFi] 使用硬编码WiFi凭证
[WiFi] 尝试连接: 你的WiFi名称
.....
[WiFi] ✅ 连接成功！IP: 192.168.1.123

[MQTT] 连接中...
[MQTT] 服务器: 192.168.1.100:1883
[MQTT] ✅ 已连接
[MQTT] 订阅: ac/user_1/dev_test_esp_001/+
```

**成功！** 🎉

---

## 🔧 方法2：使用SmartConfig（需要App）

### 前置条件

- 安装"ESP Touch"或"SmartConfig" App
- 手机连接到2.4GHz WiFi

### 步骤1：修复LED闪烁

编辑 `led_indicator.cpp`，找到 `STATUS_UNCONFIGURED` 的处理：

```cpp
case STATUS_UNCONFIGURED:
  // 快速闪烁（200ms间隔）
  if (currentMillis - lastBlink >= 200) {
    lastBlink = currentMillis;
    sysLedState = !sysLedState;
    digitalWrite(PIN_LED_SYS, sysLedState);
  }
  break;
```

确保LED会快速闪烁。

### 步骤2：首次上电

ESP启动后会自动进入SmartConfig模式（LED快闪）

### 步骤3：使用App配网

1. 打开"ESP Touch" App
2. 确认手机连接的WiFi名称
3. 输入WiFi密码
4. 点击"确认"
5. 等待配网成功（约10-30秒）

### 步骤4：手动配置MQTT

还需要修改 `config.h` 中的MQTT配置（同方法1）

---

## 📝 方法3：通过串口命令配置（高级）

### 在 `ac_controller.ino` 添加串口命令处理

在 `loop()` 函数中添加：

```cpp
void loop() {
  // ... 现有代码
  
  // 串口命令处理
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("WIFI:")) {
      // 格式: WIFI:ssid,password
      int idx = cmd.indexOf(',', 5);
      if (idx > 0) {
        String ssid = cmd.substring(5, idx);
        String pass = cmd.substring(idx + 1);
        WiFiManager::saveCredentials(ssid, pass);
        DEBUG_PRINTLN("WiFi凭证已保存，重启生效");
        ESP.restart();
      }
    }
    else if (cmd.startsWith("MQTT:")) {
      // 格式: MQTT:server,port
      // TODO: 保存到config
    }
  }
}
```

### 使用方法

打开串口监视器，输入：

```
WIFI:你的WiFi名称,你的WiFi密码
```

发送后ESP会重启并连接WiFi。

---

## 🔍 后端通信流程说明

### 你的困惑：后端怎么自动下发配置？

**答案**：后端**不会自动**下发配置给新设备！

### 实际流程

```
┌─────────┐         ┌─────────┐         ┌─────────┐
│  ESP    │         │  MQTT   │         │ 后端API │
└────┬────┘         └────┬────┘         └────┬────┘
     │                   │                   │
     │  1. 连接MQTT      │                   │
     ├──────────────────>│                   │
     │                   │                   │
     │  2. 订阅Topic     │                   │
     ├──────────────────>│                   │
     │  ac/user_1/dev_test_esp_001/+        │
     │                   │                   │
     │                   │  3. ESP上报状态   │
     │  发布status       ├──────────────────>│
     ├──────────────────>│                   │
     │                   │                   │
     │                   │  4. 用户通过App   │
     │                   │     发送控制命令  │
     │  接收cmd          │<──────────────────┤
     │<──────────────────┤                   │
     │                   │                   │
     │  5. 执行红外发送  │                   │
     │                   │                   │
```

### 关键点

1. **ESP主动连接MQTT**（使用 `config.h` 中的地址）
2. **ESP订阅自己的Topic**：`ac/user_{userId}/dev_{uuid}/+`
3. **后端通过MQTT发送命令**到这个Topic
4. **配置下发**是通过 `ac/user_{userId}/dev_{uuid}/config` Topic

### 首次绑定设备流程

```
1. 用户在App中点击"添加设备"
2. 后端创建device记录（分配deviceId）
3. 用户输入ESP的UUID（从串口查看，或贴在设备上）
4. 后端通过MQTT发送配置：
   Topic: ac/user_1/dev_test_esp_001/config
   Payload: {"userId": 1, "deviceUUID": "test_esp_001"}
5. ESP收到配置，保存到EEPROM
```

**但测试环境下**：你直接在 `config.h` 硬编码了 `USER_ID` 和 `DEVICE_UUID`，跳过了这一步！

---

## ✅ 推荐的测试流程

### 阶段1：ESP单独测试

```cpp
// config.h
#define WIFI_SSID "你的WiFi"
#define WIFI_PASSWORD "你的密码"
#define MQTT_SERVER "192.168.1.100"  // 本地MQTT broker
#define MQTT_PORT 1883
#define USER_ID 1
#define DEVICE_UUID "test_esp_001"
```

上传固件，查看串口：
- ✅ WiFi连接成功
- ✅ MQTT连接成功
- ✅ 订阅Topic成功

### 阶段2：后端集成测试

启动后端：
```bash
cd ac-iot-server
npm run start:dev
```

后端会连接到同一个MQTT broker。

### 阶段3：手动发送MQTT命令测试

使用MQTT.fx或MQTTX工具：

**发送控制命令**：
```
Topic: ac/user_1/dev_test_esp_001/cmd
Payload: {"power":1,"mode":1,"temp":26,"fan":0}
```

**查看ESP串口**：
```
[MQTT] 收到消息: Topic=ac/user_1/dev_test_esp_001/cmd
[主程序] → 收到控制命令
[红外] 发送品牌协议: GREE (型号: 0)
[红外] ✅ 品牌协议发送成功
```

**启动自动检测**：
```
Topic: ac/user_1/dev_test_esp_001/auto_detect
Payload: {"action":"start"}
```

按遥控器，查看结果上报！

### 阶段4：后端API测试

使用Postman：

```bash
# 1. 创建设备
POST http://localhost:3000/devices
{
  "name": "测试空调",
  "uuid": "test_esp_001"
}

# 2. 发送控制命令
POST http://localhost:3000/devices/1/cmd
{
  "power": true,
  "mode": "cool",
  "temp": 26
}

# 3. 启动自动检测
POST http://localhost:3000/devices/1/auto-detect/start
```

---

## 📊 完整配置示例

### config.h（测试配置）

```cpp
// ===== WiFi配置 =====
#define WIFI_SSID "TP-LINK_HOME"
#define WIFI_PASSWORD "12345678"
#define WIFI_CONNECT_TIMEOUT 20000
#define WIFI_RECONNECT_DELAY 5000

// ===== MQTT配置 =====
#define MQTT_SERVER "192.168.1.100"  // 本地电脑IP
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASSWORD "admin123"

// 设备标识
#define DEVICE_UUID "test_esp_001"
#define USER_ID 1

// MQTT连接参数
#define MQTT_KEEPALIVE 60
#define MQTT_RECONNECT_DELAY 5000
#define MQTT_BUFFER_SIZE 512
```

### 后端 `.env`（对应配置）

```env
MQTT_BROKER_URL=mqtt://192.168.1.100:1883
MQTT_USERNAME=admin
MQTT_PASSWORD=admin123
```

---

## 🐛 常见问题

### Q1: ESP连不上WiFi

**检查**：
- WiFi名称和密码是否正确
- 路由器是否支持2.4GHz（ESP8266不支持5GHz）
- ESP距离路由器是否太远

### Q2: MQTT连接失败

**检查**：
- 后端MQTT服务是否启动（`npm run start:dev`）
- IP地址是否正确（不要用localhost，用实际IP）
- 防火墙是否放行1883端口

**测试MQTT**：
```bash
# 安装MQTT客户端
npm install -g mqtt

# 手动连接测试
mqtt sub -h 192.168.1.100 -p 1883 -t 'ac/#' -v
```

### Q3: LED不闪烁

**修改 led_indicator.cpp**：

找到 `STATUS_UNCONFIGURED` case，确保有闪烁逻辑：

```cpp
case STATUS_UNCONFIGURED:
  if (currentMillis - lastBlink >= 200) {  // 200ms快闪
    lastBlink = currentMillis;
    sysLedState = !sysLedState;
    digitalWrite(PIN_LED_SYS, sysLedState);
  }
  break;
```

---

## 总结

**测试环境最简单的方法**：

1. ✅ **修改 `config.h`**：硬编码WiFi和MQTT
2. ✅ **修改 `wifi_manager.cpp`**：优先使用硬编码凭证
3. ✅ **上传固件**
4. ✅ **启动后端** (`npm run start:dev`)
5. ✅ **用MQTTX测试**发送命令
6. ✅ **用Postman测试**API

**不需要App、不需要SmartConfig、不需要前端！** 🚀
