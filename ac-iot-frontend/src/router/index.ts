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
]

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    routes,
})

// 路由守卫
router.beforeEach((to, from, next) => {
    const token = localStorage.getItem('access_token')

    if (to.meta.requiresAuth && !token) {
        next('/login')
    } else {
        next()
    }
})

export default router
