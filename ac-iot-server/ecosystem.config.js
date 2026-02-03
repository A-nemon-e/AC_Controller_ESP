module.exports = {
    apps: [
        {
            name: 'ac-iot-server',
            script: './dist/main.js',
            cwd: '/opt/ac-iot-server/AC_Controller_ESP/ac-iot-server',

            // 环境变量
            env: {
                NODE_ENV: 'production',
                PORT: 3000
            },

            // 实例数量 (集群模式)
            instances: 1,  // 改为'max'使用所有CPU核心
            exec_mode: 'fork',  // 或'cluster'启用集群模式

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
