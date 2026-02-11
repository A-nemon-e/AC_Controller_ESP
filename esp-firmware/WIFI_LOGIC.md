# WiFi 逻辑与连接策略

本文档详细介绍了 `WiFiManager` 中实现的 WiFi 连接逻辑。系统使用健壮的 **3层连接策略** 以确保可靠性和易用性。

## 3层连接策略

设备按以下顺序尝试连接 WiFi：

### Layer 1: 硬编码凭证 (高优先级)
*   **来源**: `config.h` 中定义的宏 `WIFI_SSID` 和 `WIFI_PASSWORD`。
*   **行为**:
    1.  检查是否定义了 `WIFI_SSID`。
    2.  尝试使用这些凭证进行连接。
    3.  **重试**: 2 次尝试。
    4.  **超时**: 每次尝试约 22.5 秒 (总计约 45 秒)。
*   **成功**: 设置 `configured = true` 并 **将凭证保存到 EEPROM** (更新 Layer 2)。
*   **失败**: 进入 Layer 2。

### Layer 2: EEPROM 凭证 (中优先级)
*   **来源**: 从 EEPROM 地址 0 (SSID) 和 32 (密码) 读取。
*   **行为**:
    1.  从 EEPROM 读取 SSID 和密码。
    2.  验证简单完整性 (长度检查，无 0xFF)。
    3.  尝试连接。
    4.  **重试**: 2 次尝试。
    5.  **超时**: 每次尝试约 15 秒 (总计约 30 秒)。
*   **成功**: 设置 `configured = true`。
*   **失败**: 进入 Layer 3。

### Layer 3: AP 模式 + Captive Portal (Fallback)
*   **模式**: 仅接入点 (AP) 模式。
*   **SSID**: `ESP_Setup_XXXXXX` (其中 XXXXXX 是芯片 ID)。
*   **IP 地址**: `192.168.4.1`。
*   **行为**:
    *   启动 SoftAP。
    *   **Captive Portal (强制门户)**: 在端口 53 启动 DNS 服务器。
        *   所有 DNS 请求 (通配符 `*`) 都被解析为 `192.168.4.1`。
        *   在 Android/iOS 设备上触发“登录网络”提示。
    *   **Web 服务器**: 在端口 80 启动。
        *   在 `/` 提供配置页面。
        *   将所有 404/未知请求重定向到 `/` 以支持 Captive Portal 检测探测。
    *   **用户操作**: 用户在 Web 表单中输入 SSID 和密码，然后点击“Save & Restart”。
    *   **结果**: 凭证保存到 EEPROM (Layer 2)，设备重启以尝试从 Layer 2 连接。

## 连接流程图

```mermaid
graph TD
    Start[启动] --> L1{Layer 1: 硬编码?}
    L1 -- 是 --> ConnectL1[尝试 L1 连接]
    L1 -- 否 --> L2
    
    ConnectL1 -- 成功 --> SaveEEPROM[保存到 EEPROM]
    SaveEEPROM --> Connected[已连接 (Station 模式)]
    ConnectL1 -- 失败 --> L2{Layer 2: EEPROM?}
    
    L2 -- 数据有效 --> ConnectL2[尝试 L2 连接]
    L2 -- 空/无效 --> L3
    
    ConnectL2 -- 成功 --> Connected
    ConnectL2 -- 失败 --> L3[Layer 3: AP 模式]
    
    L3 --> StartAP[启动 SoftAP & DNS]
    StartAP --> WaitUser[等待 Web 配网]
    WaitUser -- 保存 --> Reboot[重启 ESP]
    Reboot --> Start
```

## 关键组件

### `WiFiManager::connect()`
`setup()` 期间调用的主入口点。它编排了 3 层逻辑。

### `WiFiManager::attemptConnection(ssid, password, timeout)`
辅助函数：
1.  启动 `WiFi.begin()`。
2.  等待 `WL_CONNECTED` 状态。
3.  强制执行指定的 `timeout` (毫秒)。
4.  如果连接成功返回 `true`，超时返回 `false`。

### `WiFiManager::startAPMode()`
阻塞函数：
1.  设置 `WiFi.mode(WIFI_AP)`。
2.  启动 `DNSServer` 用于 Captive Portal。
3.  启动 `ESP8266WebServer`。
4.  进入 `while(true)` 循环处理 DNS 和 Web 客户端请求，直到用户保存配置并重启设备。
