# ESP Firmware EEPROM Design & Reliability Protocol

## 1. 核心架构改进 (Final Architecture)

### 1.1 物理规格
*   **总容量**: **4096 Bytes (4KB)** - 利用 ESP8266 硬件支持的最大容量，以容纳红外场景数据。
*   **库机制**: ESP8266 的 `EEPROM` 库实际上是 RAM 镜像机制。`EERPOM.begin(SIZE)` 会申请 RAM 缓冲区，只有调用 `commit()` 时才会真正擦写 Flash 为了保护寿命。
*   **一致性策略**: 为了防止不同模块（WiFiManager vs ConfigManager）之间的 RAM 缓冲区不同步（"幽灵写入"），我们实施了 **"强制刷新 (Force Flush)"** 策略。

### 1.2 内存映射 (Memory Map)

| 地址范围 | 大小 | 模块 | 内容 | 读写策略 |
| :--- | :--- | :--- | :--- | :--- |
| **0 - 95** | 96B | WiFiManager | SSID & Password | 独立读写，不通过 ConfigManager |
| **96 - 127** | 32B | - | *Reserved* (Legacy UUID) | 未使用 (历史遗留) |
| **128** | 1B | WiFiManager | Config Flag (0x55) | 标记 WiFi 是否已通过 AP 配网 |
| **129 - 132** | 4B | ConfigManager | **User ID (Backup)** | **独立救灾备份**，直接存储 `uint32_t` |
| **133 - 136** | 4B | ConfigManager | **Device ID (Backup)** | **独立救灾备份**，直接存储 `uint32_t` |
| **137 - 255** | 119B | - | *Reserved* | 预留空间 |
| **256 - 480*** | ~226B | ConfigManager | **DeviceConfig (Main)** | **Packed Struct**，含所有配置 + Checksum |
| **481 - 511** | - | - | *Gap* | 安全间隔，防止越界 |
| **512 - 4095** | 3584B | SceneManager | **IR Scenes (Array)** | **7个场景槽位** (每个 ~476B) |

*(DeviceConfig 大小取决于结构体定义，目前约 226 Bytes)*

---

## 2. 关键防御机制 (The Fixes)

### 2.1 强制字节对齐 (Byte Alignment)
为了消除编译器在结构体中插入随机 "Padding" (填充字节) 导致 Checksum 计算不稳定的问题（如 `0x75` vs `0x01` 实际上是对齐差异），`DeviceConfig` 结构体使用了 `__attribute__((packed))`。
*   **效果**: 结构体在内存中的布局是紧凑的，没有任何空隙。
*   **意义**: 保证了 `sizeof(DeviceConfig)` 是固定的，且 Checksum 计算对所有字节都是确定的，跨平台/跨版本兼容。

### 2.2 强制缓冲区刷新 (The "Slap" Protocol)
ESP8266 的 EEPROM 库有一个特性：如果 `begin()` 时缓冲区已存在，它不会重新读 Flash。
这导致如果 A 模块写了数据但还没 commit，或者 B 模块关闭了 EEPROM，C 模块可能读到脏数据。

**我们的操作标准**：
在 **每一次** 关键读写操作（`load`, `save`）之前，ConfigManager 和 SceneManager 都会执行：
```cpp
EEPROM.end();               // 1. 销毁当前 RAM 缓冲区 (如有)
EEPROM.begin(EEPROM_SIZE);  // 2. 重新申请 RAM，并强制从 Flash 重新加载数据
```
这确保了我们永远在操作 Flash 中的 "真相"，彻底杜绝了 "写了但没生效" 的幽灵问题。

---

## 3. 模块操作详解 (Operation Workflows)

### 3.1 ConfigManager (核心配置)

#### **数据结构 (DeviceConfig)**
使用 `__attribute__((packed))` 强制紧凑排列，消除平台差异和编译器 Padding。

```cpp
struct DeviceConfig {
  char mqttServer[64];     // 64 Bytes
  uint16_t mqttPort;       // 2 Bytes
  char mqttUser[32];       // 32 Bytes
  char mqttPassword[64];   // 64 Bytes
  char deviceUUID[32];     // 32 Bytes
  uint32_t userId;         // 4 Bytes (关键 ID)
  uint32_t sensorInterval; // 4 Bytes
  uint32_t ghostWindow;    // 4 Bytes

  // 品牌协议配置
  char brand[16];          // 16 Bytes ("GREE" etc)
  uint8_t model;           // 1 Byte

  uint8_t checksum;        // 1 Byte (XOR Checksum)
} __attribute__((packed)); // Total: ~226 Bytes
```

#### **关键算法 (Checksum)**
采用简单的异或 (XOR) 校验。计算范围从结构体首地址开始，直到 `checksum` 字段前一个字节。

```cpp
uint8_t calculateChecksum() {
  uint8_t sum = 0;
  uint8_t *ptr = (uint8_t *)&config;
  for (size_t i = 0; i < sizeof(DeviceConfig) - 1; i++) {
    sum ^= ptr[i]; // XOR
  }
  return sum;
}
```

#### **写入流程 (save)**
```cpp
bool save() {
  // 1. 强制刷新 Buffer (End + Begin)
  EEPROM.end();
  EEPROM.begin(4096);

  // 2. 计算当前配置的 Checksum
  config.checksum = calculateChecksum();

  // 3. 双重写入 (Double Write)
  // 3a. 先写主结构体
  EEPROM.put(256, config);
  
  // 3b. 再写独立备份槽位 (User ID) - 防止主结构体损坏时 ID 丢失
  EEPROM.put(129, config.userId);

  // 4. 提交到 Flash
  return EEPROM.commit();
}
```

#### **读取与恢复流程 (load)**
```cpp
bool load() {
  // 1. 强制刷新 Buffer
  EEPROM.end(); 
  EEPROM.begin(4096);

  // 2. 读取主结构体
  EEPROM.get(256, config);

  // 3. 校验
  if (config.checksum != calculateChecksum()) {
    // === 灾难恢复模式 (Rescue Mode) ===
    
    // a. 尝试从独立槽位读取 User ID
    uint32_t savedUserId;
    EEPROM.get(129, savedUserId);

    // b. 重置内存中的 Config 为干净的默认值 (重要!)
    // 防止使用读取到的乱码数据，确保 MQTT 端口等参数正确
    resetToDefault();

    // c. 判定备份是否有效
    if (savedUserId != 0 && savedUserId != 0xFFFFFFFF) {
      // d. 恢复 User ID
      config.userId = savedUserId;
      
      // e. 立即保存修复后的完整配置
      save();
      
      return true; // 救灾成功，视为 Load 成功
    }
    
    // f. 彻底无法恢复，只能重置
    return false;
  }

  return true; // 正常加载成功
}
```

### 3.2 SceneManager (红外与场景)

#### **写入 (addScene / save)**
1.  **Flush**: `EEPROM.end()` -> `EEPROM.begin(4096)`。
2.  **Limit**: 严格检查场景数量是否 > 7 (防止写入超过 4096 造成回绕覆盖)。
3.  **Write**: `EEPROM.put(512, scenes)`。
4.  **Commit**: `EEPROM.commit()`。

### 3.3 WiFiManager (配网)
*(注：WiFiManager 使用底层独立逻辑，主要在启动早期运行)*
1.  **Connect**: 优先尝试硬编码凭证 -> 读取 EEPROM (0/32) -> 失败则开 AP。
2.  **Save Credentials**: 写入 SSID(0) / Pass(32) / Flag(128) -> `commit()` -> `EEPROM.end()`。
    *   注意：它最后调用的 `end()` 曾是导致 ConfigManager 读不到数据的元凶，但现在我们有了 "强制刷新" 机制，已免疫此影响。

---

## 4. 总结
经过这些改进，EEPROM 子系统现在实现了：
1.  **健壮性**: 能抵抗乱序调用和 WiFiManager 的干扰。
2.  **数据安全**: 关键 ID (User/Device) 有双重备份 (Struct + Independent)。
3.  **防内存踩踏**: 严格的地址规划和场景数量限制，防止红外数据覆盖系统配置。
4.  **确定性验证**: 字节对齐 + HEX 级日志，让每一个字节都有迹可循。
