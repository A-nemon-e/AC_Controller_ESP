# ESP12F空调控制器固件 - 阶段一完成

## 📦 已创建的文件

### 配置文件
- `config.h` - 所有硬件引脚、WiFi/MQTT配置和系统参数

### LED指示模块
- `led_indicator.h` - LED控制接口
- `led_indicator.cpp` - LED状态指示实现（4种状态模式）

### WiFi管理模块  
- `wifi_manager.h` - WiFi连接接口
- `wifi_manager.cpp` - SmartConfig配网、EEPROM凭证存储、自动重连

### MQTT客户端模块
- `mqtt_client.h` - MQTT通信接口
- `mqtt_client.cpp` - 连接、订阅、发布、自动重连

### 主程序
- `ac_controller.ino` - 主程序入口，整合所有模块

---

##  ✅ 阶段一功能清单

- [x] 项目结构创建
- [x] 引脚配置定义
- [x] LED状态指示系统
  - [x] 未配网（快闪200ms）
  - [x] WiFi连接中（慢闪1000ms）
  - [x] MQTT连接中（快闪500ms）
  - [x] 就绪状态（常亮）
  - [x] 红外接收指示（100ms闪烁）
- [x] WiFi管理
  - [x] SmartConfig配网
  - [x] EEPROM凭证存储
  - [x] 自动重连（5秒间隔）
- [x] MQTT客户端
  - [x] 连接MQTT Broker
  - [x] Topic自动生成（`ac/user_{userId}/dev_{uuid}/{suffix}`）
  - [x] 订阅3个topic（cmd、learn/start、config）
  - [x] 消息回调处理
  - [x] 自动重连（5秒间隔）
- [x] 调试输出系统

---

## 📝 使用说明

### 1. 配置参数

编辑 `config.h` 文件，修改以下参数：

```cpp
// MQTT服务器地址
#define MQTT_SERVER   "your-mqtt-server.com"
#define MQTT_PORT     1883
#define MQTT_USER     "esp_device"
#define MQTT_PASSWORD "your_password"

// 设备UUID（每个设备唯一）
#define DEVICE_UUID   "esp_001"
#define USER_ID       1
```

### 2. 安装Arduino库

需要安装以下库：
- `ESP8266WiFi` (ESP8266核心自带)
- `PubSubClient` (MQTT客户端)

### 3. 上传固件

1. 打开 `ac_controller.ino`
2. 选择开发板：`Generic ESP8266 Module` 或 `NodeMCU 1.0`
3. 上传固件

### 4. 首次配网

1. 首次启动会自动进入SmartConfig模式
2. LED快闪（200ms）表示等待配网
3. 使用ESP Touch等APP进行配网
4. 配网成功后LED会慢闪（1000ms）表示WiFi连接中
5. 连接成功后LED快闪（500ms）表示MQTT连接中
6. 全部连接成功后LED常亮

### 5. 查看调试输出

打开串口监视器（115200波特率）：

```
========================================
  ESP12F 空调控制器
  版本: v1.0.0 (阶段一)
========================================

[LED] 初始化完成
[WiFi] 初始化WiFi模块
[WiFi] ✅ 连接成功！IP: 192.168.1.100
[MQTT] ✅ 连接成功
[系统信息]
  芯片ID: 0x12AB34CD
  MAC地址: AA:BB:CC:DD:EE:FF
  IP地址: 192.168.1.100
  空闲内存: 45000 bytes
  设备UUID: esp_001
  用户ID: 1

========================================
  系统初始化完成！
========================================
```

---

## 🧪 验证方法

### 测试1：WiFi连接
1. 上传固件并重启
2. 观察LED状态变化
3. 串口应显示WiFi连接成功和IP地址

### 测试2：MQTT连接
1. 在MQTT服务器控制台订阅：`ac/user_1/dev_esp_001/#`
2. 应该收到上线消息：`{"online":true}`

### 测试3：MQTT订阅
查看串口输出，应显示：
```
[MQTT] 订阅: ac/user_1/dev_esp_001/cmd
[MQTT] 订阅: ac/user_1/dev_esp_001/learn/start
[MQTT] 订阅: ac/user_1/dev_esp_001/config
```

### 测试4：消息接收
向 `ac/user_1/dev_esp_001/cmd` 发送测试消息：
```json
{"test": "hello"}
```

串口应显示：
```
[MQTT] 收到消息: ac/user_1/dev_esp_001/cmd
[MQTT] 内容: {"test": "hello"}
[主程序] 处理MQTT消息
[主程序] → 收到控制命令
```

---

## 🔧 内存使用

当前代码编译后大约使用：
- Flash: ~300KB
- RAM: ~35KB（空闲约45KB）

---

## 下一步（阶段二）

- [ ] 集成AHT20传感器
- [ ] 实现ADC电流采样
- [ ] 红外接收功能
- [ ] 红外发送功能
- [ ] 状态上报MQTT
