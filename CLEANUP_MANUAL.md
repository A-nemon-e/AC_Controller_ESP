# GitHub上传前的手动清理指南

> ⚠️ 由于你的项目在OneDrive中，自动脚本可能失败，建议手动清理

---

## 📋 必须删除的文件/文件夹

### 1. 后端目录 (`ac-iot-server`)

进入 `ac-iot-server` 文件夹，**手动删除**以下内容：

- [ ] `node_modules` 文件夹 (很大，约200-300MB)
- [ ] `dist` 文件夹 (编译产物)
- [ ] `*.db` 文件 (数据库文件):
  - `local_dev.db`
  - `ac-controller.db`
- [ ] `*.log` 文件 (日志)
- [ ] 测试脚本:
  - `test_*.js`
  - `mock_*.js`
  - `check_*.js`
  - `verify_*.js`
- [ ] `package-lock.json` (可选，建议保留锁定版本)

### 2. ESP固件目录 (`esp-firmware/ac_controller`)

通常不需要清理，但检查是否有：

- [ ] `*.hex`, `*.elf`, `*.map` (Arduino编译产物，如果有)

### 3. 系统临时文件（整个项目）

**可选清理**：

- [ ] `.DS_Store` (macOS)
- [ ] `Thumbs.db`, `desktop.ini` (Windows)
- [ ] `*.swp`, `*.swo` (Vim临时文件)

---

## 🎯 快速清理步骤

### Windows手动清理（推荐）

1. **打开文件资源管理器**
2. **导航到**: `C:\Users\xc\OneDrive\文档\AC_Controller_ESP\ac-iot-server`
3. **删除以下文件夹** (右键 → 删除):
   ```
   node_modules
   dist
   ```
4. **删除以下文件**:
   - 找到所有 `.db` 文件 → 删除
   - 找到所有 `.log` 文件 → 删除
   - 找到 `test_*.js`, `mock_*.js` 等 → 删除

5. **检查项目大小**:
   - 清理前: ~300MB+
   - 清理后: ~50MB左右 ✅

---

## ✅ 验证清理结果

### 检查 .gitignore 是否生效

在项目根目录运行：

```powershell
# 查看将要提交的文件
git status

# 应该看不到 node_modules, dist, *.db 等
```

### 如果仍然看到不应该提交的文件

```powershell
# 强制移除已追踪的文件
git rm -r --cached node_modules
git rm -r --cached dist
git rm --cached *.db

# 重新添加
git add .
```

---

## 📦 上传到GitHub

### 方式1: 使用Git命令行（推荐）

```powershell
# 1. 检查状态
git status

# 2. 添加所有文件
git add .

# 3. 提交
git commit -m "Initial commit: ESP8266 AC Controller with NestJS backend"

# 4. 添加远程仓库（首次）
git remote add origin https://github.com/your-username/AC_Controller_ESP.git

# 5. 推送
git push -u origin main
# 或者 git push -u origin master
```

### 方式2: 使用GitHub Desktop

1. 打开 GitHub Desktop
2. Add Local Repository → 选择项目文件夹
3. 查看 Changes 列表，确认没有 `node_modules`, `*.db` 等
4. 写提交信息
5. Commit to main
6. Publish repository

---

## 🗂 应该上传的文件清单

### ✅ 应该包含

- ✅ 源代码 (`.ts`, `.cpp`, `.h`, `.ino`)
- ✅ 配置文件 (`package.json`, `tsconfig.json`, `config.h`)
- ✅ 文档 (`README*.md`, `DEPLOY*.md`)
- ✅ PM2配置 (`ecosystem.config.js`)
- ✅ `.gitignore`

### ❌ 不应该包含

- ❌ `node_modules` (太大)
- ❌ `dist` (可重新编译)
- ❌ `*.db` (用户数据)
- ❌ `.env.production` (包含敏感信息)
- ❌ 日志文件
- ❌ 测试脚本

---

## 🔍 常见问题

### Q1: 文件太多，无法全部上传？

**A**: 确保 `.gitignore` 正确配置，并清理了 `node_modules`

### Q2: 提示"文件已被跟踪"？

**A**: 运行清除缓存命令:
```powershell
git rm -r --cached .
git add .
git commit -m "Fix .gitignore"
```

### Q3: OneDrive同步慢/卡住？

**A**: 
1. 暂停OneDrive同步
2. 等待清理完成
3. 恢复同步

### Q4: 推送失败"文件太大"？

**A**: 检查是否有大文件:
```powershell
# 查找大于1MB的文件
Get-ChildItem -Recurse | Where-Object { $_.Length -gt 1MB } | Select-Object FullName, @{N="Size(MB)";E={[math]::Round($_.Length/1MB,2)}}
```

---

## 📊 推荐的项目结构（清理后）

```
AC_Controller_ESP/
├── .git/
├── .gitignore                 ← 新增
├── README.md                  ← 新增
├── cleanup.ps1                ← 新增
│
├── ac-iot-server/
│   ├── src/                   ← 源代码
│   ├── package.json           ← 依赖定义
│   ├── tsconfig.json
│   ├── ecosystem.config.js    ← 新增
│   ├── DEPLOY_DEBIAN12.md     ← 新增
│   └── DEPLOY_QUICK_START.md  ← 新增
│
└── esp-firmware/
    └── ac_controller/
        ├── *.ino, *.cpp, *.h  ← 固件源码
        ├── config.h
        └── README*.md         ← 文档
```

**预计大小**: 30-50MB ✅

---

## 🎉 完成清理后

上传成功后，你的仓库将包含：

- ✅ 完整的源代码
- ✅ 详细的文档
- ✅ 部署配置
- ✅ 只有必要的文件

别人克隆后只需：

```bash
# 后端
cd ac-iot-server
npm install      # 重新安装依赖
npm run build    # 重新编译

# ESP固件
直接用Arduino IDE打开即可
```

**准备上传了吗？祝一切顺利！** 🚀
