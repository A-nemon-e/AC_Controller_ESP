import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
    {
        path: '/login',
        name: 'Login',
        component: () => import('@/views/Login.vue'),
    },
    {
        path: '/',
        component: () => import('@/layouts/TabLayout.vue'),
        meta: { requiresAuth: true },
        redirect: '/control',
        children: [
            {
                path: 'control',
                name: 'Control',
                component: () => import('@/views/Control.vue'),
                meta: { title: '控制', icon: 'fire' },
            },
            {
                path: 'schedule',
                name: 'Schedule',
                component: () => import('@/views/Schedule.vue'),
                meta: { title: '定时', icon: 'clock-o' },
            },
            {
                path: 'routine',
                name: 'Routine',
                component: () => import('@/views/Routine.vue'),
                meta: { title: '日程', icon: 'notes-o' },
            },
            {
                path: 'settings',
                name: 'Settings',
                component: () => import('@/views/Settings.vue'),
                meta: { title: '设置', icon: 'setting-o' },
            },
        ],
    },
    {
        path: '/settings/display',
        name: 'DisplaySettings',
        component: () => import('@/views/DisplaySettings.vue'),
        meta: { requiresAuth: true, title: '显示设置' },
    },
    {
        path: '/settings/weather',
        name: 'WeatherSettings',
        component: () => import('@/views/WeatherSettings.vue'),
        meta: { requiresAuth: true, title: '天气设置' },
    },
    {
        path: '/admin',
        component: () => import('@/views/admin/AdminLayout.vue'),
        meta: { requiresAuth: true, requiresAdmin: true },
        redirect: '/admin/dashboard',
        children: [
            {
                path: 'dashboard',
                name: 'AdminDashboard',
                component: () => import('@/views/admin/Dashboard.vue'),
                meta: { title: '仪表盘' },
            },
            {
                path: 'users',
                name: 'UserManagement',
                component: () => import('@/views/admin/UserManagement.vue'),
                meta: { title: '用户管理' },
            },
            {
                path: 'devices',
                name: 'DeviceManagement',
                component: () => import('@/views/admin/DeviceManagement.vue'),
                meta: { title: '设备管理' },
            },
            {
                path: 'ota',
                name: 'OtaManagement',
                component: () => import('@/views/admin/OtaManagement.vue'),
                meta: { title: 'OTA管理' },
            },
        ],
    },
]

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    routes,
})

// 路由守卫
router.beforeEach((to, _from, next) => {
    const token = localStorage.getItem('access_token')

    if (to.meta.requiresAuth && !token) {
        next('/login')
    } else {
        next()
    }
})

export default router
