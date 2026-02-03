# AC Controller ESP - 基于ESP8266的智能空调控制器

> 完整的IoT解决方案：ESP8266固件 + NestJS后端 + 自动协议检测

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Node.js](https://img.shields.io/badge/node-%3E%3D20.0.0-brightgreen.svg)](https://nodejs.org/)
[![Arduino](https://img.shields.io/badge/arduino-ESP8266-00979D.svg)](https://www.arduino.cc/)

---

## 📋 项目简介

这是一个完整的智能空调控制系统，基于ESP8266微控制器，支持红外协议自动识别、远程控制、定时任务、传感器监控等功能。

### ✨ 核心特性

#### ESP固件
- ✅ **自动协议检测**: 红外信号自动识别空调品牌和型号
- ✅ **30+品牌支持**: 格力、美的、大金、海尔等主流品牌
- ✅ **学习模式**: 不支持的品牌可通过Raw学习
- ✅ **Ghost检测**: 自动识别物理遥控器操作
- ✅ **传感器支持**: 温湿度(AHT20) + 电流(SCT-013)
- ✅ **WiFi配置**: SmartConfig + 硬编码双模式
- ✅ **MQTT通信**: 低延迟实时控制

#### 后端服务
- ✅ **RESTful API**: 设备管理、用户认证、控制命令
- ✅ **WebSocket**: 实时状态推送
- ✅ **MQTT集成**: 设备消息代理
- ✅ **定时任务**: Cron表达式支持
- ✅ **规则引擎**: 自动化场景联动
- ✅ **SQLite数据库**: 轻量级部署
- ✅ **JWT认证**: 安全的用户权限管理

---

## 📁 项目结构

```
AC_Controller_ESP/
├── esp-firmware/              # ESP8266固件
│   └── ac_controller/
│       ├── ac_controller.ino  # 主程序
│       ├── auto_detect.cpp    # 自动协议检测
│       ├── ir_controller.cpp  # 红外控制
│       ├── mqtt_client.cpp    # MQTT客户端
│       ├── config.h           # 配置文件
│       └── README*.md         # 固件文档
│
├── ac-iot-server/             # NestJS后端服务
│   ├── src/
│   │   ├── auth/              # 认证模块
│   │   ├── devices/           # 设备管理
│   │   ├── routines/          # 定时任务
│   │   ├── mqtt/              # MQTT服务
│   │   └── uplink/            # 设备消息处理
│   ├── DEPLOY_DEBIAN12.md     # 部署文档
│   └── ecosystem.config.js    # PM2配置
│
└── docs/                      # 项目文档（可选）
```

---

## 🚀 快速开始

### 1. ESP固件部署

#### 硬件需求
- ESP12F/ESP8266模组
- 红外接收头 (1838B)
- 红外LED (940nm)
- 温湿度传感器 AHT20 (可选)

#### 软件准备
1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)
2. 添加ESP8266开发板支持
3. 安装库：`IRremoteESP8266`, `PubSubClient`, `ArduinoJson`

#### 配置WiFi和MQTT

编辑 [`esp-firmware/ac_controller/config.h`](esp-firmware/ac_controller/config.h):

```cpp
#define WIFI_SSID "你的WiFi名称"
#define WIFI_PASSWORD "你的WiFi密码"
#define MQTT_SERVER "your-server-ip"
#define MQTT_PORT 1883
```

#### 上传固件

```bash
# Arduino IDE
1. 打开 ac_controller.ino
2. 选择开发板: Generic ESP8266 Module
3. 点击上传
```

详细教程: [README_USER_GUIDE.md](esp-firmware/ac_controller/README_USER_GUIDE.md)

---

### 2. 后端服务部署

#### 系统要求
- Debian 12 / Ubuntu 22.04
- Node.js 20 LTS
- Mosquitto (MQTT Broker)
- PM2

#### 快速部署

```bash
# 1. 安装依赖
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt install -y nodejs build-essential mosquitto
npm install -g pm2

# 2. 部署代码
cd /opt
git clone <your-repo-url> ac-iot-server
cd ac-iot-server/ac-iot-server

# 3. 安装依赖并编译
npm install --production
npm run build

# 4. 配置环境
cat > .env.production << 'EOF'
DB_FILE=/opt/ac-iot-server/data/ac_data.db
MQTT_URL=mqtt://localhost:1883
PORT=3000
NODE_ENV=production
JWT_SECRET=your-random-secret-key
EOF

mkdir -p /opt/ac-iot-server/data

# 5. 启动服务
pm2 start ecosystem.config.js
pm2 save
pm2 startup systemd
```

详细教程: [DEPLOY_DEBIAN12.md](ac-iot-server/DEPLOY_DEBIAN12.md)

---

## 📖 文档索引

### ESP固件
- [用户使用指南](esp-firmware/ac_controller/README_USER_GUIDE.md) - 完整的固件使用教程
- [测试环境配置](esp-firmware/ac_controller/README_TEST_CONFIG.md) - 快速测试配置
- [编译说明](esp-firmware/ac_controller/README_COMPILE.md) - 固件编译指南
- [自动检测测试](docs/auto_detect_testing_guide.md) - 协议自动检测功能测试

### 后端服务
- [Debian 12部署](ac-iot-server/DEPLOY_DEBIAN12.md) - 完整的服务器部署文档
- [快速部署](ac-iot-server/DEPLOY_QUICK_START.md) - 5分钟快速部署
- [API文档](ac-iot-server/API.md) - RESTful API接口文档（待补充）

---

## 🎯 功能演示

### 自动协议检测

```bash
# 1. 通过API启动检测
POST /devices/1/auto-detect/start

# 2. ESP进入检测模式（30秒）
# 3. 按下遥控器任意键
# 4. ESP自动识别：
#    - 协议: GREE
#    - 型号: YAC
#    - 状态: 制冷26度
# 5. 配置自动保存，完成！
```

### 远程控制

```bash
# API控制
POST /devices/1/cmd
{
  "power": true,
  "mode": "cool",
  "temp": 26,
  "fan": 0
}

# MQTT控制
Topic: ac/user_1/dev_esp_001/cmd
Payload: {"power":1,"mode":1,"temp":26}
```

---

## 🔧 开发

### ESP固件开发

```bash
# 使用PlatformIO
cd esp-firmware/ac_controller
pio init
pio run --target upload
pio device monitor
```

### 后端开发

```bash
cd ac-iot-server

# 开发模式
npm run start:dev

# 运行测试
npm run test

# 代码格式化
npm run format
```

---

## 🛠 技术栈

| 组件 | 技术 |
|------|------|
| **固件** | C++ (Arduino), ESP8266, IRremoteESP8266 |
| **后端** | NestJS, TypeScript, TypeORM, SQLite |
| **通信** | MQTT (Mosquitto), WebSocket |
| **部署** | PM2, Nginx, Systemd |

---

## 📊 系统架构

```
┌─────────────┐
│   用户App   │
└──────┬──────┘
       │ HTTP/WebSocket
       ↓
┌─────────────────┐
│  NestJS后端     │
│  - RESTful API  │
│  - WebSocket    │
│  - 定时任务     │
└────────┬────────┘
         │ MQTT
    ┌────┴────┐
    │ MQTT    │
    │ Broker  │
    └────┬────┘
         │ MQTT
    ┌────┴────────┐
    │   ESP8266   │
    │  - 红外收发  │
    │  - 传感器    │
    │  - WiFi     │
    └─────────────┘
         │ 红外
    ┌────┴────┐
    │   空调   │
    └─────────┘
```

---

## 🤝 贡献

欢迎提交Issue和Pull Request！

### 开发规范
- 代码风格: ESLint + Prettier
- 提交规范: Conventional Commits
- 分支策略: Git Flow

---

## 📄 开源协议

MIT License - 详见 [LICENSE](LICENSE) 文件

---

## 🆘 获取帮助

- **Issues**: [GitHub Issues](https://github.com/your-username/AC_Controller_ESP/issues)
- **文档**: 查看 `README*.md` 系列文档
- **邮箱**: your-email@example.com

---

## 🎉 致谢

- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) - 红外库
- [NestJS](https://nestjs.com/) - 后端框架
- [Mosquitto](https://mosquitto.org/) - MQTT Broker

---

**⭐ 如果这个项目对你有帮助，请给个Star！**
