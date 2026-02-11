# EEPROM 内存布局

本文档描述了空调控制器 ESP 固件的 EEPROM 布局。
EEPROM 大小定义为 **4096 字节** (`EEPROM_SIZE`)。

## 内存映射 (Memory Map)

| 地址范围 | 大小 (字节) | 描述 | 类型 | 使用模块 |
| :--- | :--- | :--- | :--- | :--- |
| **0x0000 - 0x001F** | 32 | WiFi SSID | `char[32]` | `WiFiManager` |
| **0x0020 - 0x005F** | 64 | WiFi 密码 | `char[64]` | `WiFiManager` |
| **0x0060 - 0x0080** | 33 | *保留 / 空隙* | - | - |
| **0x0081 - 0x0084** | 4 | 用户 ID (备份) | `uint32_t` | `ConfigManager` |
| **0x0085 - 0x0088** | 4 | 设备 ID (备份) | `uint32_t` | `ConfigManager` |
| **0x0089 - 0x00FF** | 119 | *保留 / 空隙* | - | - |
| **0x0100 - ...** | ~224 | 设备配置 | `DeviceConfig` | `ConfigManager` |
| **... - 0x1000** | - | *空闲空间* | - | - |

*(注: 0x100 = 256)*

## 详细描述

### 1. WiFi 凭证 (0x00 - 0x5F)
存储用于 Layer 2 连接的 WiFi 凭证。
*   **SSID (0-31)**: 以空字符结尾的字符串。
*   **密码 (32-95)**: 以空字符结尾的字符串。
*   **操作**:
    *   **读取**: `WiFiManager::loadCredentials` - 直接从这些地址读取。
    *   **写入**: `WiFiManager::saveCredentials` - 在 Layer 1 或 Layer 3 连接成功后写入这些地址。

### 2. 身份备份 (0x81 - 0x88)
关键身份信息的冗余存储，用于在配置损坏时恢复。
*   **用户 ID (129-132)**: `uint32_t`。存储用户 ID。
*   **设备 ID (133-136)**: `uint32_t`。存储设备 ID。
*   **操作**:
    *   **写入**: `ConfigManager::save` 和 `ConfigManager::resetToDefault` 会在更新主配置的同时更新这些值。
    *   **读取**: `ConfigManager::load` 在主配置校验和失败时读取 `User ID`，尝试恢复身份信息。

### 3. 设备配置 (0x100 - ...)
存储带有校验和的主应用程序配置结构体 (`DeviceConfig`)。
**基地址**: 256 (`EEPROM_CONFIG_ADDR`)

#### 结构体布局 (`struct DeviceConfig`)
| 偏移量 | 字段 | 类型 | 大小 | 描述 |
| :--- | :--- | :--- | :--- | :--- |
| **0** | `mqttServer` | `char[64]` | 64 | MQTT 服务器地址 |
| **64** | `mqttPort` | `uint16_t` | 2 | MQTT 服务器端口 |
| **66** | `mqttUser` | `char[32]` | 32 | MQTT 用户名 |
| **98** | `mqttPassword` | `char[64]` | 64 | MQTT 密码 |
| **162** | `deviceUUID` | `char[32]` | 32 |以此设备唯一 ID |
| **194** | `userId` | `uint32_t` | 4 | 用户 ID |
| **198** | `sensorInterval` | `uint32_t` | 4 | 传感器上报间隔 (ms) |
| **202** | `ghostWindow` | `uint32_t` | 4 | Ghost 检测窗口 (ms) |
| **206** | `brand` | `char[16]` | 16 | 空调品牌 (如 "GREE") |
| **222** | `model` | `uint8_t` | 1 | 空调型号代码 |
| **223** | `checksum` | `uint8_t` | 1 | 前面所有字节的 XOR 校验和 |

*   **操作**:
    *   **读取**: `ConfigManager::load` - 使用 `EEPROM.get` 读取整个结构体，并验证 `checksum`。
    *   **写入**: `ConfigManager::save` - 计算新的校验和并使用 `EEPROM.put` 写入整个结构体。
