# AC IoT Frontend

基于 Vue 3 + Vite + TypeScript + Vant 的空调物联网控制系统前端。

## ✨ 特性

- 🎨 **Vue 3** - 使用 Composition API
- 📱 **Vant** - 移动端 UI 组件库
- 🎭 **Pinia** - 轻量级状态管理
- 🚀 **Vite** - 极速开发体验
- 🔐 **JWT认证** - 安全的用户认证
- 🔌 **WebSocket** - 实时设备状态更新
- 📊 **ECharts** - 数据可视化

## 📦 安装

```bash
npm install
```

## 🚀 开发

```bash
npm run dev
```

访问 http://localhost:5173

## 🏗️ 构建

```bash
npm run build
```

## 📱 功能模块

### 1. 控制 Tab
- 温湿度实时显示
- 遥控器风格控制界面
- 开关机/模式/温度/风速/摆风控制
- 实时状态同步

### 2. 定时 Tab
- 创建定时任务
- 支持每天/工作日/周末重复
- 开关机和温度设置

### 3. 日程 Tab
- Routine 自动化规则
- 多触发器条件（温度/湿度/时间等）
- AND/OR 逻辑组合

### 4. 设置 Tab
- 设备管理
- 手动添加设备
- 自动发现设备
- 用户设置

## 🔧 配置

### 环境变量

**开发环境** (`.env.development`):
```
VITE_API_URL=http://localhost:3000
VITE_WS_URL=http://localhost:3000
```

**生产环境** (`.env.production`):
```
VITE_API_URL=
VITE_WS_URL=
```

## 📂 目录结构

```
src/
├── api/              # API接口
├── components/       # 通用组件
├── composables/      # 组合式函数
├── constants/        # 常量定义
├── layouts/          # 布局组件
├── router/           # 路由配置
├── stores/           # Pinia stores
├── styles/           # 全局样式
├── types/            # TypeScript类型
├── utils/            # 工具函数
├── views/            # 页面组件
├── App.vue           # 根组件
└── main.ts           # 入口文件
```

## � 后端连接

前端通过以下方式连接后端：

1. **HTTP API**: Axios + JWT Token
2. **WebSocket**: Socket.io实时通信

确保后端服务运行在 `http://localhost:3000`

## 📝 默认账号

- 用户名: `admin`
- 密码: `admin123`

## � 待完成功能

- [ ] 设备详情页面
- [ ] 数据图表展示
- [ ] 操作日志查看
- [ ] 用户设置页面
- [ ] 国际化支持

## 📄 License

MIT
