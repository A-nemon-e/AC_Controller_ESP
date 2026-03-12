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
| Phase 1: 硬件抽象层 | ✅ | ✅ | ✅ | 完成 |
| Phase 2: 显示引擎 | ✅ | ⏳ | ⏳ | 设计中 |
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

### 8.2 目录结构速查

```
AC_Controller_ESP/
├── PROJECT_DESIGN.md          # 项目设计
├── SYSTEM_DESIGN.md           # 系统设计
├── CODE_ANALYSIS.md           # 代码分析
├── PROJECT_DOCS_GUIDE.md      # 本文档
├── README.md                  # 项目简介
├── esp-firmware/
│   ├── docs/
│   │   ├── PHASE1_DESIGN.md
│   │   ├── PHASE2_DESIGN.md
│   │   └── PHASE1_TEST_REPORT.md
│   ├── lib/
│   └── test_phase1/
├── ac-iot-server/
│   └── docs/
│       ├── PHASE3_DESIGN.md
│       └── PHASE4_DESIGN.md
└── ac-iot-frontend/
    └── docs/
        └── PHASE5_DESIGN.md
```

---

**最后更新**: 2026-03-12  
**维护者**: AI Agent Team  
**文档版本**: v1.0
