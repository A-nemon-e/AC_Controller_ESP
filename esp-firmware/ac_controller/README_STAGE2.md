# ESP固件 - 阶段二完成

## ✅ 已实现功能

### 传感器模块 (sensors.h/.cpp)
- ✅ AHT20温湿度传感器读取（I2C）
- ✅ ADC电流采样（多次平均）
- ✅ 定时MQTT上报（可配置间隔）
- ✅ JSON格式状态消息

### 红外控制器 (ir_controller.h/.cpp)
- ✅ 原始时序数据发送（`sendRaw`）
- ✅ 红外信号接收和解码
- ✅ 支持UNKNOWN协议（自定义空调）
- ✅ 原始数据转换（字符串↔数组）
- ✅ MQTT事件发布
- ✅ LED指示器集成

### 主程序更新
- ✅ 集成传感器和红外模块
- ✅ MQTT命令处理（支持raw数据）
- ✅ 定时传感器上报循环
- ✅ 红外接收处理循环

---

## 📡 MQTT消息格式

### 上行消息（设备 → 服务器）

#### 1. 传感器状态（每30秒）
```json
Topic: ac/user_{userId}/dev_{uuid}/status
{
  "temp": 26.5,
  "hum": 60.2,
  "current": 1.23,
  "timestamp": 123456
}
```

#### 2. 红外接收事件
```json
Topic: ac/user_{userId}/dev_{uuid}/event
{
  "source": "ir_recv",
  "protocol": "UNKNOWN",
  "value": "0x12AB34CD",
  "bits": 32,
  "raw": "9000,4500,560,1680,..."
}
```

### 下行消息（服务器 → 设备）

#### 1. 控制命令
```json
Topic: ac/user_{userId}/dev_{uuid}/cmd
{
  "raw": "9000,4500,560,1680,..."
}
```

> **注意**：目前仅支持`raw`模式。品牌协议解析（power, mode, temp等）将在阶段三实现。

---

## 🧪 测试方法

### 测试1：传感器读数

```bash
# 查看串口输出
[传感器] 温度: 26.5°C, 湿度: 60.2%, 电流: 1.23A
[传感器] ✅ 状态已上报
```

### 测试2：红外发送

通过MQTT发送：
```bash
mosquitto_pub -h server.com \
  -t "ac/user_1/dev_esp_001/cmd" \
  -m '{"raw":"9000,4500,560,560,560,1680"}'
```

### 测试3：红外接收

用遥控器对准ESP：
```bash
[红外] 📡 收到红外信号
[红外] 协议: UNKNOWN (自定义协议)
[红外] 原始数据: 9000,4500,560,1680,...
```

---

## 📚 所需Arduino库

- ✅ `ESP8266WiFi` (ESP8266核心)
- ✅ `PubSubClient` (MQTT)
- ✅ `ArduinoJson` (v6+)
- ✅ `IRremoteESP8266` (红外)
- ✅ **新增**: `Adafruit_AHTX0` (AHT20传感器)
- ✅ **新增**: `Adafruit_BusIO` (I2C依赖)
- ✅ **新增**: `Adafruit_Sensor` (传感器基类)

安装方法：
```
Arduino IDE → 工具 → 管理库 → 搜索并安装
```

---

## 🔜 下一步（阶段三）

- [ ] 红外学习功能
- [ ] Ghost检测（麦克风）
- [ ] 状态管理器（空调状态记忆）

当前进度：**阶段二完成** ✅
