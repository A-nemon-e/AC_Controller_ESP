# Debian 12 部署快速参考

> 完整文档: [DEPLOY_DEBIAN12.md](./DEPLOY_DEBIAN12.md)

## 🚀 5分钟快速部署

### 1. 安装依赖
```bash
# Node.js 20
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt install -y nodejs build-essential

# MQTT服务器
apt install -y mosquitto mosquitto-clients

# Nginx (可选)
apt install -y nginx

# PM2
npm install -g pm2
```

### 2. 部署代码
```bash
# 上传代码到服务器
cd /opt/ac-iot-server/ac-iot-server

# 安装依赖
npm install --production

# 编译
npm run build
```

### 3. 配置环境
```bash
cat > .env.production << 'EOF'
DB_FILE=/opt/ac-iot-server/data/ac_data.db
MQTT_URL=mqtt://localhost:1883
PORT=3000
NODE_ENV=production
JWT_SECRET=your-secret-key-here
EOF

mkdir -p /opt/ac-iot-server/data
```

### 4. 启动服务
```bash
# 使用PM2
pm2 start ecosystem.config.js
pm2 save
pm2 startup systemd
```

### 5. 验证
```bash
# 检查状态
pm2 status
curl http://localhost:3000/

# 查看日志
pm2 logs
```

## 📋 常用命令

| 功能 | 命令 |
|------|------|
| 查看状态 | `pm2 status` |
| 查看日志 | `pm2 logs ac-iot-server` |
| 重启服务 | `pm2 restart ac-iot-server` |
| 测试MQTT | `mosquitto_pub -h localhost -t 'test' -m 'hi'` |
| 重载Nginx | `nginx -s reload` |
| 数据库备份 | `cp /opt/ac-iot-server/data/ac_data.db backup.db` |

## 🔗 服务地址

- **API**: http://your-server-ip:3000
- **MQTT**: mqtt://your-server-ip:1883
- **Nginx代理**: http://your-server-ip/api

## ⚠️ 重要提醒

1. **修改JWT密钥**: `.env.production`中的`JWT_SECRET`
2. **启用MQTT认证**: 生产环境建议`mosquitto_passwd`设置密码
3. **配置防火墙**: `ufw allow 3000/tcp`
4. **定期备份数据库**: 添加cron任务

完整部署步骤和故障排除请查看 **[DEPLOY_DEBIAN12.md](./DEPLOY_DEBIAN12.md)** 📚
