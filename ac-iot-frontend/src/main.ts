import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'

// Vant样式
import 'vant/lib/index.css'
// UnoCSS
import 'virtual:uno.css'
// 自定义样式
import './styles/main.css'

const app = createApp(App)

app.use(createPinia())
app.use(router)

app.mount('#app')
