# OTA配置系统 - 使用说明

## 🎯 设计目标

解决原有设计的问题：
- ❌ MQTT服务器地址硬编码
- ❌ 设备UUID硬编码
- ❌ 用户ID硬编码
- ❌ 每个设备需要单独编译固件

✅ 新设计特性：
- ✅ 所有设备烧录同一固件
- ✅ 服务器通过MQTT下发配置
- ✅ 配置存储EEPROM，重启保留
- ✅ 基于MAC生成默认UUID

---

## 📡 配置流程

### 1. 首次启动

设备使用默认配置：
```json
{
  "mqttServer": "your-mqtt-server.com",
  "mqttPort": 1883,
  "deviceUUID": "ESP_AABBCCDDEEFF",  // 基于MAC生成
  "userId": 0,  // 未绑定
  "sensorInterval": 30000,
  "ghostWindow": 30000
}
```

设备会订阅：`ac/config/AABBCCDDEEFF`

### 2. 服务器下发配置

后端API检测到新设备上线后，向topic `ac/config/AABBCCDDEEFF` 发送：

```json
{
  "mqttServer": "your-mqtt-server.com",
  "mqttPort": 1883,
  "mqttUser": "esp_device",
  "mqttPassword": "password",
  "deviceUUID": "esp_001",
  "userId": 1,
  "sensorInterval": 30000,
  "ghostWindow": 30000
}
```

### 3. 设备确认

设备收到配置后：
1. 更新内存中的配置
2. 保存到EEPROM
3. 发布确认消息到 `ac/config_ack/AABBCCDDEEFF`：
   ```json
   {"status": "ok", "updated": true}
   ```

### 4. 重启后

设备从EEPROM加载配置，无需重新配置。

---

## 🔧 配置项说明

| 配置项 | 类型 | 说明 | 默认值 |
|--------|------|------|--------|
| mqttServer | string | MQTT服务器地址 | config.h中定义 |
| mqttPort | uint16 | MQTT端口 | 1883 |
| mqttUser | string | MQTT用户名 | config.h中定义 |
| mqttPassword | string | MQTT密码 | config.h中定义 |
| deviceUUID | string | 设备UUID | ESP_MAC地址 |
| userId | uint32 | 用户ID | 0（未绑定） |
| sensorInterval | uint32 | 传感器上报间隔（ms） | 30000 |
| ghostWindow | uint32 | Ghost检测窗口（ms） | 30000 |

---

## 📋 后端API集成

### 检测新设备

监听topic：`ac/user_0/dev_ESP_*/status`

当检测到`userId=0`的设备时，表示未绑定。

### 绑定设备

1. 用户在前端输入MAC地址（或二维码扫描）
2. 后端生成配置并发送到 `ac/config/{MAC}`
3. 等待确认消息 `ac/config_ack/{MAC}`
4. 更新数据库设备记录

### 更新配置

任何时候可以向 `ac/config/{MAC}` 发送JSON更新配置。

---

## 🧪 测试方法

### 测试1：查看默认配置

```bash
# 在串口监视器中查看启动日志
========== 设备配置 ==========
MQTT服务器: your-mqtt-server.com:1883
设备UUID: ESP_AABBCCDDEEFF
用户ID: 0
...
```

### 测试2：模拟配置更新

使用MQTT客户端发送：

```bash
mosquitto_pub -h your-server.com \
  -t "ac/config/AABBCCDDEEFF" \
  -m '{"userId":1,"deviceUUID":"esp_001"}'
```

### 测试3：验证EEPROM保存

1. 更新配置
2. 重启ESP
3. 查看日志，应显示更新后的配置

---

## 🔒 安全考虑

1. **配置校验和**：EEPROM存储时添加校验和，防止数据损坏
2. **配置Topic**：基于MAC地址，避免冲突
3. **TODO**: 添加配置加密（可选）
4. **TODO**: 添加配置签名验证（高级）

---

## 所需Arduino库

需要额外安装：
- `ArduinoJson` (v6+) - JSON解析

---

## 文件清单

- `config_manager.h` - 配置管理器头文件
- `config_manager.cpp` - 配置管理器实现
- `ac_controller.ino` - 主程序（已更新）
- `mqtt_client.cpp` - MQTT客户端（已更新使用动态配置）
