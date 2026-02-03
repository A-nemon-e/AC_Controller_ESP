# AC IoT Server - Debian 12 部署指南

> **适用于**: Debian 12 (Bookworm)  
> **后端**: NestJS + TypeORM + SQLite + MQTT  
> **更新日期**: 2026-02-03

---

## 📋 目录

1. [系统要求](#系统要求)
2. [准备工作](#准备工作)
3. [安装依赖](#安装依赖)
4. [部署后端服务](#部署后端服务)
5. [配置MQTT服务器](#配置mqtt服务器)
6. [配置反向代理(Nginx)](#配置反向代理nginx)
7. [配置进程管理(PM2)](#配置进程管理pm2)
8. [启动服务](#启动服务)
9. [验证部署](#验证部署)
10. [日常维护](#日常维护)
11. [故障排除](#故障排除)

---

## 🖥 系统要求

### 最低配置
- **CPU**: 1核
- **内存**: 1GB
- **硬盘**: 10GB
- **系统**: Debian 12 (Bookworm)

### 推荐配置
- **CPU**: 2核+
- **内存**: 2GB+
- **硬盘**: 20GB+

---

## 🔧 准备工作

### 步骤1: 更新系统

```bash
# 切换到root用户或使用sudo
sudo -i

# 更新软件包列表
apt update && apt upgrade -y

# 安装基础工具
apt install -y curl wget git vim htop
```

### 步骤2: 创建部署目录

```bash
# 创建工作目录
mkdir -p /opt/ac-iot-server
cd /opt/ac-iot-server
```

---

## 📦 安装依赖

### 1. 安装Node.js (推荐v20 LTS)

```bash
# 添加NodeSource仓库
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -

# 安装Node.js
apt install -y nodejs

# 验证安装
node --version  # 应显示 v20.x.x
npm --version   # 应显示 10.x.x
```

### 2. 安装构建工具

```bash
# SQLite3需要编译原生模块
apt install -y build-essential python3

# 验证
gcc --version
python3 --version
```

### 3. 安装MQTT服务器 (Mosquitto)

```bash
# 安装Mosquitto
apt install -y mosquitto mosquitto-clients

# 启动并设置开机自启
systemctl enable mosquitto
systemctl start mosquitto

# 验证
systemctl status mosquitto
```

### 4. 安装Nginx (可选，用于反向代理)

```bash
apt install -y nginx

systemctl enable nginx
systemctl start nginx
```

---

## 🚀 部署后端服务

### 步骤1: 上传代码

**方式A: 使用Git (推荐)**

```bash
cd /opt/ac-iot-server

# 克隆仓库 (替换为你的仓库地址)
git clone https://github.com/your-username/AC_Controller_ESP.git
cd AC_Controller_ESP/ac-iot-server
```

**方式B: 使用SCP上传**

```bash
# 在本地电脑执行 (Windows使用Git Bash或PowerShell)
scp -r C:\Users\xc\OneDrive\文档\AC_Controller_ESP\ac-iot-server root@your-server-ip:/opt/ac-iot-server/
```

**方式C: 使用FTP (FileZilla等)**

上传整个 `ac-iot-server` 文件夹到服务器 `/opt/ac-iot-server/`

### 步骤2: 安装依赖

```bash
cd /opt/ac-iot-server/ac-iot-server  # 注意路径

# 清理node_modules (如果从Windows上传)
rm -rf node_modules package-lock.json

# 安装生产依赖
npm install --production

# 如果需要编译TypeScript
npm install  # 包含devDependencies
```

### 步骤3: 配置环境变量

```bash
# 创建生产环境配置
cat > .env.production << 'EOF'
# 数据库配置
DB_FILE=/opt/ac-iot-server/data/ac_data.db

# MQTT配置
MQTT_URL=mqtt://localhost:1883
MQTT_USERNAME=
MQTT_PASSWORD=

# 服务器配置
PORT=3000
NODE_ENV=production

# JWT密钥 (⚠️ 请修改为随机字符串)
JWT_SECRET=your-super-secret-jwt-key-change-this-in-production

# 日志级别
LOG_LEVEL=info
EOF

# 设置权限
chmod 600 .env.production
```

### 步骤4: 创建数据目录

```bash
# 创建数据库存储目录
mkdir -p /opt/ac-iot-server/data

# 设置权限
chown -R root:root /opt/ac-iot-server
chmod 755 /opt/ac-iot-server/data
```

### 步骤5: 编译TypeScript

```bash
cd /opt/ac-iot-server/ac-iot-server

# 构建生产版本
npm run build

# 验证dist目录
ls -la dist/
# 应该看到main.js和其他编译后的文件
```

---

## 📡 配置MQTT服务器

### 步骤1: 配置Mosquitto

```bash
# 编辑配置文件
vim /etc/mosquitto/mosquitto.conf
```

添加以下内容：

```conf
# 监听端口
listener 1883

# 允许匿名连接 (测试环境，生产建议启用认证)
allow_anonymous true

# 日志配置
log_dest file /var/log/mosquitto/mosquitto.log
log_type all

# 数据持久化
persistence true
persistence_location /var/lib/mosquitto/
```

**生产环境建议启用认证**：

```bash
# 创建密码文件
mosquitto_passwd -c /etc/mosquitto/passwd admin

# 输入密码两次 (例如: admin123)

# 修改配置
vim /etc/mosquitto/mosquitto.conf
```

添加：

```conf
allow_anonymous false
password_file /etc/mosquitto/passwd
```

### 步骤2: 重启Mosquitto

```bash
systemctl restart mosquitto

# 验证
systemctl status mosquitto

# 测试订阅
mosquitto_sub -h localhost -t 'test/#' &

# 测试发布
mosquitto_pub -h localhost -t 'test/demo' -m 'Hello MQTT'

# 应该能看到消息输出
```

### 步骤3: 开放防火墙 (如果启用了UFW)

```bash
# 允许MQTT端口
ufw allow 1883/tcp

# 允许HTTP/HTTPS (如果使用Nginx)
ufw allow 80/tcp
ufw allow 443/tcp

# 允许SSH
ufw allow 22/tcp

# 启用防火墙
ufw enable
```

---

## 🔒 配置反向代理(Nginx)

### 为什么需要Nginx?

1. **HTTPS支持**: 自动SSL证书
2. **负载均衡**: 多实例部署
3. **静态文件服务**: 前端页面
4. **安全防护**: 隐藏内部端口

### 配置Nginx

```bash
# 创建站点配置
vim /etc/nginx/sites-available/ac-iot-server
```

添加以下内容：

```nginx
# HTTP服务器
server {
    listen 80;
    server_name your-domain.com;  # ⚠️ 替换为你的域名或IP

    # API代理
    location /api/ {
        proxy_pass http://localhost:3000/;
        proxy_http_version 1.1;
        
        # WebSocket支持
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
        
        # 请求头
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # 超时设置
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
    
    # Socket.io WebSocket (如果需要)
    location /socket.io/ {
        proxy_pass http://localhost:3000/socket.io/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
    
    # 健康检查
    location /health {
        proxy_pass http://localhost:3000/health;
        access_log off;
    }
}
```

### 启用站点

```bash
# 创建软链接
ln -s /etc/nginx/sites-available/ac-iot-server /etc/nginx/sites-enabled/

# 测试配置
nginx -t

# 重载Nginx
systemctl reload nginx
```

---

## 🔄 配置进程管理(PM2)

### 为什么使用PM2?

1. **自动重启**: 崩溃自动恢复
2. **开机自启**: 系统重启后自动运行
3. **日志管理**: 自动日志轮换
4. **集群模式**: 多核CPU利用

### 步骤1: 安装PM2

```bash
npm install -g pm2
```

### 步骤2: 创建PM2配置

```bash
cd /opt/ac-iot-server/ac-iot-server

# 创建ecosystem文件
cat > ecosystem.config.js << 'EOF'
module.exports = {
  apps: [
    {
      name: 'ac-iot-server',
      script: './dist/main.js',
      cwd: '/opt/ac-iot-server/ac-iot-server',
      
      // 环境变量
      env: {
        NODE_ENV: 'production',
        PORT: 3000
      },
      
      // 实例数量 (集群模式)
      instances: 1,  // 改为'max'使用所有CPU核心
      exec_mode: 'fork',  // 或'cluster'
      
      // 自动重启
      autorestart: true,
      watch: false,
      max_memory_restart: '500M',
      
      // 日志
      error_file: '/var/log/ac-iot-server/error.log',
      out_file: '/var/log/ac-iot-server/out.log',
      log_date_format: 'YYYY-MM-DD HH:mm:ss Z',
      merge_logs: true,
      
      // 其他
      min_uptime: '10s',
      max_restarts: 10,
      restart_delay: 4000
    }
  ]
};
EOF
```

### 步骤3: 创建日志目录

```bash
mkdir -p /var/log/ac-iot-server
```

### 步骤4: 配置开机自启

```bash
# 启动PM2管理的应用
pm2 start ecosystem.config.js

# 保存PM2进程列表
pm2 save

# 生成开机自启脚本
pm2 startup systemd

# 执行上一步输出的命令 (类似):
# sudo env PATH=$PATH:/usr/bin pm2 startup systemd -u root --hp /root
```

---

## ▶️ 启动服务

### 启动所有服务

```bash
# 1. 启动MQTT
systemctl start mosquitto

# 2. 启动Nginx (如果配置了)
systemctl start nginx

# 3. 启动后端 (使用PM2)
cd /opt/ac-iot-server/ac-iot-server
pm2 start ecosystem.config.js

# 或直接启动 (不推荐生产环境)
# NODE_ENV=production node dist/main.js
```

### 查看状态

```bash
# PM2状态
pm2 status
pm2 logs

# MQTT状态
systemctl status mosquitto

# Nginx状态
systemctl status nginx
```

---

## ✅ 验证部署

### 1. 检查端口监听

```bash
# 查看监听的端口
netstat -tlnp | grep -E '3000|1883|80'

# 应该看到:
# 0.0.0.0:3000  (NestJS)
# 0.0.0.0:1883  (MQTT)
# 0.0.0.0:80    (Nginx)
```

### 2. 测试API

```bash
# 健康检查
curl http://localhost:3000/

# 应返回: {"message":"AC IoT Server API"}

# 测试认证
curl -X POST http://localhost:3000/auth/ register \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","email":"admin@example.com"}'
```

### 3. 测试MQTT

```bash
# 订阅测试
mosquitto_sub -h localhost -t 'ac/#' -v &

# 发布测试
mosquitto_pub -h localhost -t 'ac/test' -m '{"test":true}'
```

### 4. 从外网访问

```bash
# 使用服务器公网IP
curl http://your-server-ip/api/

# 或使用域名 (如果配置了)
curl http://your-domain.com/api/
```

---

## 🔧 日常维护

### 查看日志

```bash
# PM2日志
pm2 logs ac-iot-server

# 实时查看最新日志
pm2 logs ac-iot-server --lines 100

# MQTT日志
tail -f /var/log/mosquitto/mosquitto.log

# Nginx日志
tail -f /var/log/nginx/access.log
tail -f /var/log/nginx/error.log
```

### 重启服务

```bash
# 重启后端
pm2 restart ac-iot-server

# 重启MQTT
systemctl restart mosquitto

# 重载Nginx配置
nginx -s reload
```

### 更新代码

```bash
cd /opt/ac-iot-server/ac-iot-server

# 拉取最新代码
git pull

# 安装依赖
npm install --production

# 重新编译
npm run build

# 重启服务
pm2 restart ac-iot-server
```

### 数据库备份

```bash
# 创建备份脚本
cat > /opt/backup-db.sh << 'EOF'
#!/bin/bash
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="/opt/backups"
DB_FILE="/opt/ac-iot-server/data/ac_data.db"

mkdir -p $BACKUP_DIR
cp $DB_FILE $BACKUP_DIR/ac_data_$DATE.db

# 保留最近7天的备份
find $BACKUP_DIR -name "ac_data_*.db" -mtime +7 -delete

echo "Backup completed: ac_data_$DATE.db"
EOF

chmod +x /opt/backup-db.sh

# 添加到cron (每天凌晨2点备份)
crontab -e
# 添加: 0 2 * * * /opt/backup-db.sh
```

### 监控资源使用

```bash
# 查看PM2监控
pm2 monit

# 系统资源
htop

# 磁盘使用
df -h

# 内存使用
free -h
```

---

## ❗ 故障排除

### 问题1: 端口3000已被占用

```bash
# 查找占用进程
lsof -i :3000

# 杀死进程
kill -9 <PID>

# 或修改端口
vim .env.production
# PORT=3001
```

### 问题2: MQTT连接失败

```bash
# 检查Mosquitto状态
systemctl status mosquitto

# 查看日志
journalctl -u mosquitto -f

# 测试本地连接
mosquitto_pub -h localhost -t 'test' -m 'hi'

# 检查防火墙
ufw status
```

### 问题3: 数据库锁定

```bash
# SQLite可能被锁定
cd /opt/ac-iot-server/data

# 检查文件权限
ls -la ac_data.db

# 停止应用重新初始化
pm2 stop ac-iot-server
rm -f ac_data.db ac_data.db-*
pm2 start ac-iot-server
```

### 问题4: PM2无法启动

```bash
# 删除PM2缓存
pm2 delete all
pm2 kill

# 重新启动
pm2 start ecosystem.config.js
pm2 save
```

### 问题5: 内存不足

```bash
# 查看内存使用
free -h

# 添加swap (如果没有)
fallocate -l 2G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile

# 永久启用swap
echo '/swapfile none swap sw 0 0' >> /etc/fstab
```

---

## 📊 性能优化

### 1. 启用PM2集群模式

```javascript
// ecosystem.config.js
instances: 'max',  // 使用所有CPU核心
exec_mode: 'cluster'
```

### 2. 配置Nginx缓存

```nginx
# /etc/nginx/sites-available/ac-iot-server
http {
    proxy_cache_path /var/cache/nginx levels=1:2 keys_zone=api_cache:10m max_size=100m inactive=60m;
    
    server {
        location /api/devices {
            proxy_cache api_cache;
            proxy_cache_valid 200 5m;
            # ...
        }
    }
}
```

### 3. 数据库优化

```bash
# 定期VACUUM清理SQLite
sqlite3 /opt/ac-iot-server/data/ac_data.db "VACUUM;"
```

---

## 🎯 快速部署脚本

创建一键部署脚本：

```bash
cat > /opt/deploy.sh << 'EOF'
#!/bin/bash
set -e

echo "🚀 开始部署AC IoT Server..."

# 1. 更新代码
cd /opt/ac-iot-server/ac-iot-server
git pull

# 2. 安装依赖
npm install --production

# 3. 编译
npm run build

# 4. 重启服务
pm2 restart ac-iot-server

# 5. 验证
sleep 5
pm2 status
curl -f http://localhost:3000/ || echo "❌ API未响应"

echo "✅ 部署完成！"
EOF

chmod +x /opt/deploy.sh
```

使用：

```bash
/opt/deploy.sh
```

---

## 📚 相关文档

- [NestJS部署文档](https://docs.nestjs.com/deployment)
- [PM2文档](https://pm2.keymetrics.io/)
- [Mosquitto文档](https://mosquitto.org/documentation/)
- [Nginx文档](https://nginx.org/en/docs/)

---

## 🆘 获取帮助

- **GitHub Issues**: 项目仓库Issues页面
- **日志文件**: `/var/log/ac-iot-server/`
- **PM2日志**: `pm2 logs`

**部署成功后，你的服务器将在以下地址可访问**：

- **API**: `http://your-server-ip:3000` 或 `http://your-domain.com/api`
- **MQTT**: `mqtt://your-server-ip:1883`
- **WebSocket**: `ws://your-server-ip:3000/socket.io`

🎉 **祝部署顺利！**
