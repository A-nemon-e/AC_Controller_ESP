# AC IoT 全栈生产部署指南

> **适用于**: Debian 12 / Ubuntu 22.04  
> **前端**: Vue 3 + Vite  
> **后端**: NestJS + SQLite + MQTT  
> **更新日期**: 2026-02-03

---

## 📋 部署架构

```
用户浏览器
    ↓ HTTPS (443)
Apache2/Nginx (反向代理 + 静态文件服务)
    ├── /                → 前端 (Vue 3静态文件)
    ├── /api/*           → 后端API (NestJS:3000)
    └── /socket.io/*     → WebSocket (NestJS:3000)

ESP设备
    ↓ MQTT (1883)
Mosquitto MQTT Broker
    ↓
NestJS后端 (localhost:3000)
```

---

## 🚀 快速部署步骤

### 1. 准备服务器环境

```bash
# 更新系统
apt update && apt upgrade -y

# 安装基础工具
apt install -y curl wget git vim htop

# 安装Node.js 20
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt install -y nodejs build-essential

# 安装MQTT服务器
apt install -y mosquitto mosquitto-clients

# 安装PM2
npm install -g pm2

# 选择一个Web服务器
# 选项A: Apache2
apt install -y apache2

# 选项B: Nginx
apt install -y nginx
```

### 2. 部署后端

```bash
# 假设代码已在服务器
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-server

# 安装依赖
npm install --production

# 编译
npm run build

# 创建数据目录
mkdir -p /opt/ac-iot-server/AC_Controller_ESP/data

# 配置环境变量
cat > .env.production << 'EOF'
DB_FILE=/opt/ac-iot-server/AC_Controller_ESP/data/ac_data.db
MQTT_URL=mqtt://localhost:1883
PORT=3000
NODE_ENV=production
JWT_SECRET=CHANGE_THIS_TO_RANDOM_STRING_IN_PRODUCTION
LOG_LEVEL=info
EOF

chmod 600 .env.production
```

### 3. 构建和部署前端

**方式A: 服务器端构建（推荐）**

```bash
# 在服务器上执行
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-frontend

# 安装依赖
npm install

# 构建生产版本（已优化，跳过类型检查）
npm run build

# 创建Web根目录
mkdir -p /var/www/ac-iot-frontend

# 复制构建产物
cp -r dist/* /var/www/ac-iot-frontend/

# 设置权限
chmod -R 755 /var/www/ac-iot-frontend/
```

**方式B: 本地Windows构建后上传**

```bash
# 在本地Windows电脑执行
cd C:\Users\xc\OneDrive\文档\AC_Controller_ESP\ac-iot-frontend
npm install
npm run build

# 上传到服务器
scp -r dist/* root@your-server-ip:/var/www/ac-iot-frontend/
```

---

## 🔧 Apache2 配置 (选项A)

### 1. 启用必要模块

```bash
a2enmod proxy
a2enmod proxy_http
a2enmod proxy_wstunnel
a2enmod rewrite
a2enmod headers
a2enmod ssl  # 如果需要HTTPS
```

### 2. 创建站点配置

```bash
vim /etc/apache2/sites-available/ac-iot.conf
```

**完整配置内容**：

```apache
<VirtualHost *:80>
    ServerName your-domain.com  # ⚠️ 替换为你的域名或IP
    ServerAdmin admin@your-domain.com

    # 日志
    ErrorLog ${APACHE_LOG_DIR}/ac-iot-error.log
    CustomLog ${APACHE_LOG_DIR}/ac-iot-access.log combined

    # 前端静态文件根目录
    DocumentRoot /var/www/ac-iot-frontend

    <Directory /var/www/ac-iot-frontend>
        Options -Indexes +FollowSymLinks
        AllowOverride All
        Require all granted
        
        # Vue Router HTML5 History模式支持
        <IfModule mod_rewrite.c>
            RewriteEngine On
            RewriteBase /
            RewriteRule ^index\.html$ - [L]
            RewriteCond %{REQUEST_FILENAME} !-f
            RewriteCond %{REQUEST_FILENAME} !-d
            RewriteRule . /index.html [L]
        </IfModule>
    </Directory>

    # 静态资源缓存
    <FilesMatch "\.(js|css|jpg|jpeg|png|gif|ico|svg|woff|woff2|ttf|eot)$">
        Header set Cache-Control "max-age=31536000, public"
    </FilesMatch>

    # API代理
    ProxyPreserveHost On
    ProxyRequests Off
    
    # 后端API代理 (所有/api/*请求)
    ProxyPass /api http://localhost:3000
    ProxyPassReverse /api http://localhost:3000

    # WebSocket支持 (Socket.io)
    ProxyPass /socket.io/ http://localhost:3000/socket.io/
    ProxyPassReverse /socket.io/ http://localhost:3000/socket.io/
    
    # WebSocket升级
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} =websocket [NC]
    RewriteRule /(.*) ws://localhost:3000/$1 [P,L]

    # 请求头
    RequestHeader set X-Forwarded-Proto "http"
    RequestHeader set X-Forwarded-Port "80"
    RequestHeader set X-Real-IP %{REMOTE_ADDR}s
</VirtualHost>

# HTTPS配置 (如果有SSL证书)
<VirtualHost *:443>
    ServerName your-domain.com
    ServerAdmin admin@your-domain.com

    # SSL配置
    SSLEngine on
    SSLCertificateFile /etc/ssl/certs/your-cert.crt
    SSLCertificateKeyFile /etc/ssl/private/your-key.key
    # SSLCertificateChainFile /etc/ssl/certs/your-chain.crt  # 如果需要

    # 日志
    ErrorLog ${APACHE_LOG_DIR}/ac-iot-ssl-error.log
    CustomLog ${APACHE_LOG_DIR}/ac-iot-ssl-access.log combined

    # 前端静态文件
    DocumentRoot /var/www/ac-iot-frontend

    <Directory /var/www/ac-iot-frontend>
        Options -Indexes +FollowSymLinks
        AllowOverride All
        Require all granted
        
        <IfModule mod_rewrite.c>
            RewriteEngine On
            RewriteBase /
            RewriteRule ^index\.html$ - [L]
            RewriteCond %{REQUEST_FILENAME} !-f
            RewriteCond %{REQUEST_FILENAME} !-d
            RewriteRule . /index.html [L]
        </IfModule>
    </Directory>

    # 静态资源缓存
    <FilesMatch "\.(js|css|jpg|jpeg|png|gif|ico|svg|woff|woff2|ttf|eot)$">
        Header set Cache-Control "max-age=31536000, public"
    </FilesMatch>

    # API代理
    ProxyPreserveHost On
    ProxyRequests Off
    
    ProxyPass /api http://localhost:3000
    ProxyPassReverse /api http://localhost:3000

    ProxyPass /socket.io/ http://localhost:3000/socket.io/
    ProxyPassReverse /socket.io/ http://localhost:3000/socket.io/
    
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} =websocket [NC]
    RewriteRule /(.*) ws://localhost:3000/$1 [P,L]

    # HTTPS请求头
    RequestHeader set X-Forwarded-Proto "https"
    RequestHeader set X-Forwarded-Port "443"
    RequestHeader set X-Real-IP %{REMOTE_ADDR}s
</VirtualHost>
```

不开http的：
```apache
<VirtualHost *:80>
    ServerName a.ifelsa.uk
    # 也可以填 ServerAlias www.a.ifelsa.uk
    ServerAdmin admin@a.ifelsa.uk

    # -----------------------------------------------------------
    # 停用 HTTP 业务逻辑，强制重定向到 HTTPS
    # -----------------------------------------------------------
    RewriteEngine On
    RewriteCond %{HTTPS} off
    RewriteRule ^ https://%{HTTP_HOST}%{REQUEST_URI} [L,R=301]

    # 这里的日志可以保留，用于排查重定向问题
    ErrorLog ${APACHE_LOG_DIR}/ac-iot-error.log
    CustomLog ${APACHE_LOG_DIR}/ac-iot-access.log combined
</VirtualHost>

# HTTPS配置
<VirtualHost *:443>
    ServerName a.ifelsa.uk
    ServerAdmin admin@a.ifelsa.uk

    # -----------------------------------------------------------
    # SSL 证书配置 
    # (⚠️注意：请确认这些文件真实存在，否则 Apache 无法启动)
    # -----------------------------------------------------------
    SSLEngine on
    SSLCertificateFile /etc/letsencrypt/live/a.ifelsa.uk/fullchain.pem
    SSLCertificateKeyFile /etc/letsencrypt/live/a.ifelsa.uk/privkey.pem
    # 旧版本 Apache 可能需要 SSLCertificateChainFile，新版 Let's Encrypt 不需要

    # 日志
    ErrorLog ${APACHE_LOG_DIR}/ac-iot-ssl-error.log
    CustomLog ${APACHE_LOG_DIR}/ac-iot-ssl-access.log combined

    # -----------------------------------------------------------
    # 前端静态文件根目录
    # -----------------------------------------------------------
    DocumentRoot /var/www/ac-iot-frontend

    <Directory /var/www/ac-iot-frontend>
        Options -Indexes +FollowSymLinks
        AllowOverride All
        Require all granted
        
        # Vue Router HTML5 History模式支持
        <IfModule mod_rewrite.c>
            RewriteEngine On
            RewriteBase /
            RewriteRule ^index\.html$ - [L]
            RewriteCond %{REQUEST_FILENAME} !-f
            RewriteCond %{REQUEST_FILENAME} !-d
            RewriteRule . /index.html [L]
        </IfModule>
    </Directory>

    # 静态资源缓存
    <FilesMatch "\.(js|css|jpg|jpeg|png|gif|ico|svg|woff|woff2|ttf|eot)$">
        Header set Cache-Control "max-age=31536000, public"
    </FilesMatch>

    # -----------------------------------------------------------
    # 后端 API 代理配置
    # -----------------------------------------------------------
    ProxyPreserveHost On
    ProxyRequests Off
    
    # 后端API代理 (所有/api/*请求)
    ProxyPass /api http://localhost:3000
    ProxyPassReverse /api http://localhost:3000

    # WebSocket支持 (Socket.io)
    ProxyPass /socket.io/ http://localhost:3000/socket.io/
    ProxyPassReverse /socket.io/ http://localhost:3000/socket.io/
    
    # WebSocket升级逻辑
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} =websocket [NC]
    RewriteRule /(.*) ws://localhost:3000/$1 [P,L]

    # -----------------------------------------------------------
    # HTTPS 专用请求头
    # -----------------------------------------------------------
    RequestHeader set X-Forwarded-Proto "https"
    RequestHeader set X-Forwarded-Port "443"
    RequestHeader set X-Real-IP %{REMOTE_ADDR}s
</VirtualHost>
```

### 3. 启用站点

```bash
# 禁用默认站点
a2dissite 000-default.conf

# 启用新站点
a2ensite ac-iot.conf

# 测试配置
apache2ctl configtest

# 重载Apache
systemctl reload apache2
```

### 4. 配置Let's Encrypt免费SSL (可选)

```bash
# 安装Certbot
apt install -y certbot python3-certbot-apache

# 自动配置SSL
certbot --apache -d your-domain.com

# 自动续期
certbot renew --dry-run
```

---

## 🔧 Nginx 配置 (选项B)

### 1. 创建站点配置

```bash
vim /etc/nginx/sites-available/ac-iot
```

**完整配置内容**：

```nginx
# HTTP服务器
server {
    listen 80;
    server_name your-domain.com;  # ⚠️ 替换为你的域名或IP

    # 日志
    access_log /var/log/nginx/ac-iot-access.log;
    error_log /var/log/nginx/ac-iot-error.log;

    # 前端静态文件根目录
    root /var/www/ac-iot-frontend;
    index index.html;

    # Gzip压缩
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_types text/plain text/css text/xml text/javascript 
               application/json application/javascript application/xml+rss 
               application/x-javascript application/xml application/xhtml+xml;

    # 前端路由 (Vue Router History模式)
    location / {
        try_files $uri $uri/ /index.html;
    }

    # 静态资源缓存
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # API代理
    location /api {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    # WebSocket代理 (Socket.io)
    location /socket.io/ {
        proxy_pass http://localhost:3000/socket.io/;
        proxy_http_version 1.1;
        
        # WebSocket升级
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # 超时设置
        proxy_connect_timeout 7d;
        proxy_send_timeout 7d;
        proxy_read_timeout 7d;
    }

    # 健康检查
    location /health {
        proxy_pass http://localhost:3000/health;
        access_log off;
    }
}

# HTTPS服务器 (如果有SSL证书)
server {
    listen 443 ssl http2;
    server_name your-domain.com;

    # SSL证书配置
    ssl_certificate /etc/ssl/certs/your-cert.crt;
    ssl_certificate_key /etc/ssl/private/your-key.key;
    
    # SSL优化
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;
    ssl_session_cache shared:SSL:10m;
    ssl_session_timeout 10m;

    # 日志
    access_log /var/log/nginx/ac-iot-ssl-access.log;
    error_log /var/log/nginx/ac-iot-ssl-error.log;

    # 前端静态文件
    root /var/www/ac-iot-frontend;
    index index.html;

    # Gzip压缩
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_types text/plain text/css text/xml text/javascript 
               application/json application/javascript application/xml+rss 
               application/x-javascript application/xml application/xhtml+xml;

    # 前端路由
    location / {
        try_files $uri $uri/ /index.html;
    }

    # 静态资源缓存
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # API代理
    location /api {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto https;
        
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    # WebSocket代理
    location /socket.io/ {
        proxy_pass http://localhost:3000/socket.io/;
        proxy_http_version 1.1;
        
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto https;
        
        proxy_connect_timeout 7d;
        proxy_send_timeout 7d;
        proxy_read_timeout 7d;
    }

    # 健康检查
    location /health {
        proxy_pass http://localhost:3000/health;
        access_log off;
    }
}
```

### 2. 启用站点

```bash
# 创建软链接
ln -s /etc/nginx/sites-available/ac-iot /etc/nginx/sites-enabled/

# 删除默认站点
rm -f /etc/nginx/sites-enabled/default

# 测试配置
nginx -t

# 重载Nginx
systemctl reload nginx
```

### 3. 配置Let's Encrypt免费SSL (可选)

```bash
# 安装Certbot
apt install -y certbot python3-certbot-nginx

# 自动配置SSL
certbot --nginx -d your-domain.com

# 自动续期
certbot renew --dry-run
```

---

## 📱 前端环境变量配置

创建生产环境变量文件：

```bash
# 在前端项目根目录
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-frontend

# 创建 .env.production
cat > .env.production << 'EOF'
# 生产环境 - 使用相对路径（同源）
VITE_API_URL=
VITE_WS_URL=
EOF
```

**说明**:
- 空字符串表示使用相对路径
- 前端通过 `/api/*` 访问后端
- 通过 `/socket.io/*` 连接WebSocket
- 反向代理自动转发到后端 `localhost:3000`

---

## ▶️ 启动所有服务

```bash
# 1. 启动MQTT服务器
systemctl start mosquitto
systemctl enable mosquitto

# 2. 启动后端 (使用PM2)
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-server
pm2 start ecosystem.config.js
pm2 save
pm2 startup systemd

# 3. 启动Web服务器
# Apache2
systemctl start apache2
systemctl enable apache2

# 或 Nginx
systemctl start nginx
systemctl enable nginx
```

---

## ✅ 验证部署

```bash
# 1. 检查端口监听
netstat -tlnp | grep -E '3000|1883|80|443'

# 应该看到:
# 0.0.0.0:3000  (NestJS)
# 0.0.0.0:1883  (Mosquitto)
# 0.0.0.0:80    (Apache/Nginx)
# 0.0.0.0:443   (Apache/Nginx, 如果配置了SSL)

# 2. 测试后端API
curl http://localhost:3000/

# 3. 测试反向代理
curl http://localhost/api/

# 4. 访问前端
# 浏览器打开: http://your-server-ip
# 或: http://your-domain.com
```

---

## 🔧 日常维护

```bash
# 查看后端日志
pm2 logs ac-iot-server

# 重启后端
pm2 restart ac-iot-server

# 查看Web服务器日志
# Apache2
tail -f /var/log/apache2/ac-iot-access.log

# Nginx
tail -f /var/log/nginx/ac-iot-access.log

# 更新前端（服务器端构建）
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-frontend
git pull  # 如果使用git
npm run build
cp -r dist/* /var/www/ac-iot-frontend/
# 无需重启，刷新浏览器即可

# 更新后端
cd /opt/ac-iot-server/AC_Controller_ESP/ac-iot-server
git pull  # 或重新上传代码
npm install --production
npm run build
pm2 restart ac-iot-server
```

---

## ⚠️ 重要提醒

1. **修改JWT密钥**: `.env.production`中的`JWT_SECRET`必须改为随机字符串
2. **配置防火墙**: 
   ```bash
   ufw allow 80/tcp
   ufw allow 443/tcp
   ufw allow 1883/tcp  # MQTT
   ufw allow 22/tcp    # SSH
   ufw enable
   ```
3. **定期备份数据库**:
   ```bash
   cp /opt/ac-iot/data/ac_data.db /backup/ac_data_$(date +%Y%m%d).db
   ```
4. **启用MQTT认证** (生产环境建议):
   ```bash
   mosquitto_passwd -c /etc/mosquitto/passwd admin
   # 修改 /etc/mosquitto/mosquitto.conf
   # allow_anonymous false
   # password_file /etc/mosquitto/passwd
   ```

---

## 📞 故障排除

### 问题1: 前端白屏

**检查**:
```bash
# 确认文件存在
ls -la /var/www/ac-iot-frontend/

# 确认index.html存在
cat /var/www/ac-iot-frontend/index.html

# 检查权限
chmod -R 755 /var/www/ac-iot-frontend/
```

### 问题2: API 404

**检查**:
```bash
# 确认后端运行
pm2 status

# 测试后端直连
curl http://localhost:3000/

# 检查代理配置
# Apache: apache2ctl configtest
# Nginx: nginx -t
```

### 问题3: WebSocket连接失败

**检查**:
```bash
# 确认模块启用 (Apache)
a2enmod proxy_wstunnel

# 查看日志
# Apache: tail -f /var/log/apache2/ac-iot-error.log
# Nginx: tail -f /var/log/nginx/ac-iot-error.log
```

---

**部署完成！** 🎉

访问: `http://your-server-ip` 或 `http://your-domain.com`
