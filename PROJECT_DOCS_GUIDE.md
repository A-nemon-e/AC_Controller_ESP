# AC Controller ESP - 项目文档指南

**项目**: LED矩阵时钟与空调控制器  
**版本**: v2.0  
**架构**: 五步法开发流程  
**作者**: AI Assistant  

---

## 1. 五步法与文档结构

本项目采用**五步法**开发流程，每个阶段都有对应的文档：

```
AC_Controller_ESP/
├── @architect/                    # 阶段1: 架构设计
│   └── (设计文档分散在各模块docs中)
├── @explorer/                     # 阶段2: 代码分析
│   └── CODE_ANALYSIS.md           # 代码分析报告
├── @plan/                         # 阶段3: 详细规划
│   ├── esp-firmware/docs/         # Phase 1-2 设计文档
│   ├── ac-iot-server/docs/        # Phase 3-4 设计文档
│   └── ac-iot-frontend/docs/      # Phase 5 设计文档
├── @build/                        # 阶段4: 实施
│   └── (源代码)
└── @reviewer/                     # 阶段5: 审查
    └── (测试报告)
```

---

## 2. 文档存放规范

### 2.1 根目录文档

| 文档 | 位置 | 内容 | 读者 |
|------|------|------|------|
| **PROJECT_DESIGN.md** | `/` | 项目总体设计、需求规格 | 所有开发者 |
| **SYSTEM_DESIGN.md** | `/` | 系统架构、接口规范 | 架构师、开发者 |
| **CODE_ANALYSIS.md** | `/` | 代码分析报告（@explorer） | 开发者 |
| **README.md** | `/` | 项目简介、快速开始 | 用户、新开发者 |
| **THIS_FILE** | `/` | 本文档 - 五步法指南 | AI Agent |

### 2.2 模块级文档

每个模块的 `docs/` 文件夹存放该模块的详细设计：

#### ESP固件 (esp-firmware/)
```
esp-firmware/
├── docs/
│   ├── PHASE1_DESIGN.md       # Phase 1: 硬件抽象层重构
│   └── PHASE2_DESIGN.md       # Phase 2: 显示引擎核心
└── lib/                       # 按Phase 1设计创建的库
    ├── IS31FL3733/
    ├── LEDMatrix/
    ├── FontRenderer/
    └── DisplayConfig/
```

#### 后端服务 (ac-iot-server/)
```
ac-iot-server/
├── docs/
│   ├── PHASE3_DESIGN.md       # Phase 3: 天气系统
│   └── PHASE4_DESIGN.md       # Phase 4: 后端改进
└── src/
    ├── weather/               # Phase 3实施
    └── admin/                 # Phase 4实施
```

#### 前端应用 (ac-iot-frontend/)
```
ac-iot-frontend/
├── docs/
│   └── PHASE5_DESIGN.md       # Phase 5: 前端完善
└── src/
    ├── components/display/    # Phase 5实施
    └── views/admin/           # Phase 5实施
```

### 2.3 测试文档

测试报告放在对应模块的 `docs/` 中：

```
esp-firmware/
├── docs/
│   ├── PHASE1_DESIGN.md
│   └── PHASE1_TEST_REPORT.md  # Phase 1测试报告
└── test_phase1/               # Phase 1测试代码
    └── test_phase1.ino
```

---

## 3. AI Agent工作指南

### 3.1 开始工作时的检查清单

当你（AI Agent）开始工作时，请按以下顺序阅读文档：

1. **阅读本文档** (PROJECT_DOCS_GUIDE.md) - 了解文档结构
2. **阅读 PROJECT_DESIGN.md** - 了解项目总体设计
3. **阅读 SYSTEM_DESIGN.md** - 了解系统架构
4. **阅读对应Phase的设计文档** - 了解当前任务细节
5. **阅读 CODE_ANALYSIS.md** - 了解代码现状（如果是维护任务）

### 3.2 五步法执行流程

#### Step 1: @architect (架构设计)
- **输入**: PROJECT_DESIGN.md, SYSTEM_DESIGN.md
- **输出**: 更新设计文档
- **文档位置**: 根目录和模块docs/

#### Step 2: @explorer (代码分析)
- **输入**: 现有源代码
- **输出**: CODE_ANALYSIS.md
- **文档位置**: 根目录/
- **关键内容**:
  - 代码质量评估
  - 可复用资产清单
  - 技术债务列表
  - 改进建议

#### Step 3: @plan (详细规划)
- **输入**: CODE_ANALYSIS.md, 设计文档
- **输出**: PHASE{n}_DESIGN.md
- **文档位置**: 对应模块的docs/
- **关键内容**:
  - 详细模块边界
  - 接口定义
  - 依赖关系图
  - 实施顺序

#### Step 4: @build (实施)
- **输入**: PHASE{n}_DESIGN.md
- **输出**: 源代码 + PHASE{n}_TEST_REPORT.md
- **代码位置**: 按模块组织
- **要求**:
  - 严格遵循设计文档
  - 代码必须通过编译
  - 创建测试代码
  - 记录测试结果

#### Step 5: @reviewer (审查)
- **输入**: 源代码 + 测试报告
- **输出**: 审查报告（可合并到测试报告中）
- **检查项**:
  - 代码质量
  - 功能完整性
  - 测试覆盖率
  - 文档同步

### 3.3 文档更新规范

**必须同步更新的情况**:
1. 修改接口定义 → 更新设计文档
2. 修改架构 → 更新PROJECT_DESIGN.md
3. 发现设计缺陷 → 更新所有相关文档
4. 完成Phase → 创建测试报告

**禁止的行为**:
- ❌ 代码和设计文档不一致
- ❌ 修改文档不说明原因
- ❌ 删除已有文档
- ❌ 在不合适的位置创建文档

---

## 4. 关键文档速查

### 4.1 如果你是新加入的AI Agent

**快速开始路径**:
```
1. 阅读本文档 (10分钟)
2. 阅读 PROJECT_DESIGN.md (20分钟)
3. 阅读 SYSTEM_DESIGN.md (20分钟)
4. 根据任务选择对应Phase设计文档
```

**不同任务的文档重点**:

| 任务类型 | 必读文档 | 选读文档 |
|----------|----------|----------|
| ESP固件开发 | PHASE1-2_DESIGN.md | CODE_ANALYSIS.md |
| 后端API开发 | PHASE3-4_DESIGN.md | SYSTEM_DESIGN.md |
| 前端页面开发 | PHASE5_DESIGN.md | API接口规范 |
| Bug修复 | 相关Phase设计文档 | CODE_ANALYSIS.md |
| 重构优化 | CODE_ANALYSIS.md | 所有设计文档 |

### 4.2 文档依赖关系

```
PROJECT_DESIGN.md
    ├── SYSTEM_DESIGN.md
    │       ├── esp-firmware/docs/PHASE1_DESIGN.md
    │       │       └── lib/ (Phase 1实施)
    │       ├── esp-firmware/docs/PHASE2_DESIGN.md
    │       │       └── lib/DisplayEngine/ (Phase 2实施)
    │       ├── ac-iot-server/docs/PHASE3_DESIGN.md
    │       ├── ac-iot-server/docs/PHASE4_DESIGN.md
    │       └── ac-iot-frontend/docs/PHASE5_DESIGN.md
    └── CODE_ANALYSIS.md (代码现状分析)
```

---

## 5. 项目状态追踪

### 5.1 Phase完成状态

在根目录的README.md或专门的STATUS.md中维护：

```markdown
## 项目状态

| Phase | 设计 | 实施 | 测试 | 状态 |
|-------|------|------|------|------|
| Phase 1: 硬件抽象层 | ✅ | ✅ | ✅ | **已完成** |
| Phase 2: 显示引擎 | ✅ | ⏳ | ⏳ | 待开始 |
| Phase 3: 天气系统 | ✅ | ⏳ | ⏳ | 待开始 |
| Phase 4: 后端改进 | ✅ | ⏳ | ⏳ | 待开始 |
| Phase 5: 前端完善 | ✅ | ⏳ | ⏳ | 待开始 |
| Phase 6: OTA系统 | ✅ | ⏳ | ⏳ | 待开始 |
```

### 5.2 Todo管理

使用 `.config/opencode/AGENTS.md` 中定义的Todo格式：

```markdown
- [x] 已完成任务
- [ ] 待办任务
- [⏳] 进行中任务
```

---

## 6. 编码规范

### 6.1 文档规范

- **命名**: 使用大写和下划线 `PHASE{n}_{TYPE}.md`
- **格式**: Markdown with GitHub Flavored Markdown
- **语言**: 中文为主，代码注释可中英文混合
- **结构**: 必须包含目录、版本、作者信息

### 6.2 代码规范

- **ESP固件**: 遵循Arduino风格
- **后端**: 遵循NestJS风格
- **前端**: 遵循Vue 3风格
- **注释**: 必须包含Doxygen风格文档注释

---

## 7. 联系与反馈

### 7.1 问题报告

如果你（AI Agent）在工作过程中发现：
- 文档不一致
- 设计缺陷
- 代码质量问题
- 需要架构调整

请在相应文档中添加TODO标记：
```markdown
## TODO
- [ ] 发现设计问题：XXX (日期, 发现者)
```

### 7.2 版本历史

| 日期 | 版本 | 修改内容 | 作者 |
|------|------|----------|------|
| 2026-03-12 | v1.0 | 初始版本 | AI Assistant |

---

## 8. 附录

### 8.1 常用命令

```bash
# 编译ESP固件
cd AC_Controller_ESP/esp-firmware
arduino-cli compile --libraries libraries --fqbn esp8266:esp8266:nodemcuv2 test_phase1

# 编译后端
cd AC_Controller_ESP/ac-iot-server
npm run build

# 编译前端
cd AC_Controller_ESP/ac-iot-frontend
npm run build
```

### 8.3 前端部署到Apache（生产环境）

**目标服务器**: 10.0.10.13:8222  
**Apache端口**: 6077  
**部署用户**: apache.root

#### 步骤1: 编译前端
```bash
cd AC_Controller_ESP/ac-iot-frontend
npm install
npm run build
# 生成目录: dist/ 或 build/
```

#### 步骤2: 查看Apache配置（在服务器上执行）
```bash
ssh -p 8222 apache.root@10.0.10.13
# 输入密码（查看密码管理工具）

# 查找Apache配置文件
grep -r "6077" /etc/apache2/sites-enabled/
grep -r "DocumentRoot" /etc/apache2/sites-enabled/

# 常见路径示例:
# /etc/apache2/sites-enabled/001-ac-frontend.conf
# DocumentRoot /var/www/ac-frontend/html
```

#### 步骤3: 部署前端文件
```bash
# 在服务器上执行
sudo systemctl stop apache2

# 备份旧版本
cd /var/www/ac-frontend/html
sudo tar -czf backup_$(date +%Y%m%d_%H%M%S).tar.gz .

# 清空旧文件
sudo rm -rf /var/www/ac-frontend/html/*

# 复制新文件（从本地scp到服务器）
# 在本地执行:
scp -P 8222 -r AC_Controller_ESP/ac-iot-frontend/dist/* apache.root@10.0.10.13:/var/www/ac-frontend/html/

# 设置权限
sudo chown -R www-data:www-data /var/www/ac-frontend/html
sudo chmod -R 755 /var/www/ac-frontend/html
```

#### 步骤4: 重启Apache
```bash
sudo systemctl start apache2
# 或 reload（不中断服务）
sudo systemctl reload apache2

# 检查状态
sudo systemctl status apache2
```

#### 步骤5: 验证部署
```bash
# 检查端口监听
sudo netstat -tlnp | grep 6077

# 访问测试
curl http://localhost:6077
```

**注意事项**:
- 部署前务必备份
- 确保后端服务也在运行（同服务器）
- 检查防火墙是否开放6077端口
- 如有CDN需刷新缓存

### 8.2 完整项目结构目录

```
AC_Controller_ESP/
├── PROJECT_DESIGN.md              # 项目总体设计文档
├── SYSTEM_DESIGN.md               # 系统架构设计文档
├── CODE_ANALYSIS.md               # 代码分析报告
├── PROJECT_DOCS_GUIDE.md          # 本文档 - 项目文档指南
├── README.md                      # 项目简介和快速开始
├── INTEGRATION_TEST_REPORT.md     # 集成测试报告
├── P0_FIXES_REPORT.md             # P0问题修复报告
├── P1_FIXES_REPORT.md             # P1问题修复报告
├── P2_FIXES_REPORT.md             # P2问题修复报告
├── P2_OPTIMIZATION_TODO.md        # P2优化任务清单
├── MD_HELPER_COMPLIANCE_CHECK.md  # 代码规范检查报告
│
├── esp-firmware/                  # ESP8266固件代码 (Arduino/C++)
│   ├── docs/                      # 固件设计文档
│   │   ├── PHASE1_DESIGN.md       # Phase 1: 硬件抽象层设计
│   │   ├── PHASE2_DESIGN.md       # Phase 2: 显示引擎设计
│   │   └── PHASE1_TEST_REPORT.md  # Phase 1测试报告
│   │
│   ├── lib/                       # 库文件目录
│   │   ├── IS31FL3733/            # LED驱动芯片库
│   │   │   ├── IS31FL3733.h
│   │   │   └── IS31FL3733.cpp
│   │   │
│   │   ├── LEDMatrix/             # LED矩阵管理
│   │   │   ├── LEDMatrix.h
│   │   │   ├── LEDMatrix.cpp
│   │   │   └── config_pins.h      # 引脚配置
│   │   │
│   │   ├── FontRenderer/          # 字体渲染系统
│   │   │   ├── FontRenderer.h
│   │   │   ├── FontRenderer.cpp
│   │   │   └── fonts.h            # 字体数据
│   │   │
│   │   ├── DisplayConfig/         # 显示配置管理
│   │   │   ├── DisplayConfig.h
│   │   │   └── DisplayConfig.cpp
│   │   │
│   │   ├── DisplayEngine/         # 显示引擎核心 (Phase 2)
│   │   │   └── Core/
│   │   │       ├── Background.h           # 背景基类
│   │   │       ├── Foreground.h           # 前景基类
│   │   │       ├── Card.h                 # 卡片系统
│   │   │       ├── DisplayManager.h       # 显示管理器
│   │   │       ├── Transition.h           # 转场效果
│   │   │       ├── Backgrounds/           # 背景效果实现
│   │   │       │   ├── FireBackground.h           # 火焰效果
│   │   │       │   ├── MatrixRainBackground.h     # 矩阵雨效果
│   │   │       │   ├── WaterRippleBackground.h    # 水波纹效果
│   │   │       │   ├── GameOfLifeBackground.h     # 生命游戏
│   │   │       │   ├── SandBackground.h           # 沙漏效果
│   │   │       │   └── PongBackground.h           # Pong游戏
│   │   │       └── Foregrounds/           # 前景效果实现
│   │   │           ├── ClockForeground.h          # 时钟前景
│   │   │           ├── DateForeground.h           # 日期前景
│   │   │           └── TempForeground.h           # 温湿度前景
│   │   │
│   │   ├── WeatherModule/         # 天气模块 (Phase 3)
│   │   │   ├── WeatherModule.h
│   │   │   ├── WeatherModule.cpp
│   │   │   ├── WeatherEffects.h
│   │   │   └── WeatherEffects.cpp
│   │   │
│   │   └── ButtonHandler/         # 按键处理 (Phase 1 - 待完成)
│   │       └── (待创建)
│   │
│   ├── test_phase1/               # Phase 1测试代码
│   │   └── test_phase1.ino
│   │
│   ├── test_3733_scanner/         # LED显示测试程序
│   │   ├── test_3733_scanner.ino
│   │   ├── is31fl3733.hpp
│   │   ├── is31fl3733.cpp
│   │   └── fonts.h
│   │
│   └── ac_controller/             # 空调控制器主程序
│       └── ac_controller.ino
│
├── ac-iot-server/                 # 后端服务 (NestJS/TypeScript)
│   ├── docs/                      # 后端设计文档
│   │   ├── PHASE3_DESIGN.md       # Phase 3: 天气系统设计
│   │   └── PHASE4_DESIGN.md       # Phase 4: 后端改进设计
│   │
│   ├── src/
│   │   ├── main.ts                # 应用入口
│   │   ├── app.module.ts          # 根模块
│   │   │
│   │   ├── auth/                  # 认证模块
│   │   │   ├── auth.controller.ts
│   │   │   ├── auth.service.ts
│   │   │   ├── auth.module.ts
│   │   │   ├── jwt.strategy.ts
│   │   │   └── dto/
│   │   │       ├── login.dto.ts
│   │   │       ├── register.dto.ts
│   │   │       └── change-password.dto.ts
│   │   │
│   │   ├── users/                 # 用户模块
│   │   │   ├── users.controller.ts
│   │   │   ├── users.service.ts
│   │   │   ├── users.module.ts
│   │   │   └── entities/
│   │   │       └── user.entity.ts
│   │   │
│   │   ├── devices/               # 设备管理模块
│   │   │   ├── devices.controller.ts
│   │   │   ├── devices.service.ts
│   │   │   ├── devices.module.ts
│   │   │   ├── device-binding.service.ts    # 设备绑定服务
│   │   │   └── entities/
│   │   │       └── device.entity.ts
│   │   │
│   │   ├── weather/               # 天气服务模块 (Phase 3)
│   │   │   ├── weather.module.ts
│   │   │   ├── weather.controller.ts
│   │   │   ├── weather.service.ts
│   │   │   ├── weather-scheduler.service.ts # 定时任务 (待完善)
│   │   │   └── services/
│   │   │       ├── ip-location.service.ts   # IP定位服务
│   │   │       └── heweather.service.ts     # 和风天气API
│   │   │
│   │   ├── mqtt/                  # MQTT服务
│   │   │   ├── mqtt.module.ts
│   │   │   ├── mqtt.service.ts
│   │   │   └── mqtt.controller.ts
│   │   │
│   │   ├── admin/                 # 管理后台 (Phase 4)
│   │   │   ├── admin.module.ts
│   │   │   ├── admin.controller.ts
│   │   │   ├── admin.service.ts
│   │   │   ├── admin-stats.controller.ts
│   │   │   └── admin.guard.ts     # 管理员权限守卫
│   │   │
│   │   ├── agreement/             # 用户协议模块
│   │   │   ├── agreement.service.ts
│   │   │   └── entities/
│   │   │       └── user-agreement.entity.ts
│   │   │
│   │   └── common/                # 公共模块
│   │       ├── filters/
│   │       ├── interceptors/
│   │       └── decorators/
│   │
│   ├── package.json
│   ├── tsconfig.json
│   └── nest-cli.json
│
└── ac-iot-frontend/               # 前端应用 (Vue 3/TypeScript)
    ├── docs/                      # 前端设计文档
    │   └── PHASE5_DESIGN.md       # Phase 5: 前端完善设计
    │
    ├── src/
    │   ├── main.ts                # 应用入口
    │   ├── App.vue                # 根组件
    │   │
    │   ├── views/                 # 页面视图
    │   │   ├── Login.vue          # 登录页
    │   │   ├── Register.vue       # 注册页
    │   │   ├── DisplaySettings.vue        # 显示设置 (Phase 5)
    │   │   ├── WeatherSettings.vue        # 天气设置 (Phase 3)
    │   │   ├── DeviceManagement.vue       # 设备管理
    │   │   ├── Profile.vue                # 个人中心
    │   │   └── admin/                     # 管理后台 (Phase 4)
    │   │       ├── Dashboard.vue          # 统计仪表板
    │   │       ├── UserManagement.vue     # 用户管理
    │   │       ├── DeviceManagement.vue   # 设备管理
    │   │       └── OtaManagement.vue      # OTA管理 (待完善)
    │   │
    │   ├── components/            # 可复用组件
    │   │   ├── display/           # 显示相关组件
    │   │   │   ├── CardEditor.vue         # 卡片编辑器
    │   │   │   ├── ForegroundEditor.vue   # 前景编辑器
    │   │   │   └── LEDSimulator.vue       # LED模拟器
    │   │   └── common/            # 通用组件
    │   │
    │   ├── stores/                # Pinia状态管理
    │   │   ├── auth.ts            # 认证状态
    │   │   ├── display.ts         # 显示设置状态 (TODO: API集成)
    │   │   ├── weather.ts         # 天气状态 (TODO: API集成)
    │   │   └── admin.ts           # 管理后台状态 (TODO: API集成)
    │   │
    │   ├── api/                   # API接口封装
    │   │   ├── auth.ts
    │   │   ├── devices.ts
    │   │   ├── display.ts
    │   │   ├── weather.ts
    │   │   └── admin.ts
    │   │
    │   ├── router/                # 路由配置
    │   │   └── index.ts
    │   │
    │   └── utils/                 # 工具函数
    │       └── request.ts         # HTTP请求封装
    │
    ├── package.json
    ├── vite.config.ts
    └── tsconfig.json
```

### 8.3 关键文件映射表

| 功能 | 主要文件 | 状态 |
|------|---------|------|
| **LED驱动** | `esp-firmware/lib/IS31FL3733/` | ✅ 完成 |
| **LED矩阵** | `esp-firmware/lib/LEDMatrix/` | ✅ 完成 |
| **字体渲染** | `esp-firmware/lib/FontRenderer/` | ✅ 完成 |
| **显示配置** | `esp-firmware/lib/DisplayConfig/` | ✅ 完成 |
| **显示引擎** | `esp-firmware/lib/DisplayEngine/Core/` | ⚠️ 框架完成 |
| **天气模块** | `esp-firmware/lib/WeatherModule/` | ⚠️ 80%完成 |
| **按键处理** | `esp-firmware/lib/ButtonHandler/` | ❌ 待创建 |
| **天气服务** | `ac-iot-server/src/weather/` | ⚠️ 80%完成 |
| **设备绑定** | `ac-iot-server/src/devices/device-binding.service.ts` | ✅ 完成 |
| **管理后台** | `ac-iot-server/src/admin/` | ⚠️ 70%完成 |
| **显示设置页** | `ac-iot-frontend/src/views/DisplaySettings.vue` | ✅ 完成 |
| **天气设置页** | `ac-iot-frontend/src/views/WeatherSettings.vue` | ✅ 完成 |
| **管理后台页** | `ac-iot-frontend/src/views/admin/` | ⚠️ 80%完成 |
| **前端Store** | `ac-iot-frontend/src/stores/` | ⚠️ 20%完成 (TODO) |

---

**最后更新**: 2026-03-12  
**维护者**: AI Agent Team  
**文档版本**: v1.0
