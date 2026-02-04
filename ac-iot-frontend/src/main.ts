import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'

// Vant 组件
import {
    Button,
    Form,
    Field,
    CellGroup,
    Checkbox,
    Tabbar,
    TabbarItem,
    NavBar,
    Toast,
    Dialog,
    Notify,
    Loading,
    Empty,
    Picker,
    Popup,
    DatePicker,
    TimePicker,
    Switch,
    Slider,
    Calendar,
    ActionSheet,
    Card,
    Tag,
    Icon,
    Cell,
    Radio,
    RadioGroup,
    Stepper,
    NoticeBar,
    DropdownMenu,
    DropdownItem,
    Grid,
    GridItem,
} from 'vant'

// Vant样式
import 'vant/lib/index.css'
// UnoCSS
import 'virtual:uno.css'
// 自定义样式
import './styles/main.css'

const app = createApp(App)

// 注册 Vant 组件
app.use(Button)
app.use(Form)
app.use(Field)
app.use(CellGroup)
app.use(Checkbox)
app.use(Tabbar)
app.use(TabbarItem)
app.use(NavBar)
app.use(Toast)
app.use(Dialog)
app.use(Notify)
app.use(Loading)
app.use(Empty)
app.use(Picker)
app.use(Popup)
app.use(DatePicker)
app.use(TimePicker)
app.use(Switch)
app.use(Slider)
app.use(Calendar)
app.use(ActionSheet)
app.use(Card)
app.use(Tag)
app.use(Icon)
app.use(Cell)
app.use(Radio)
app.use(RadioGroup)
app.use(Stepper)
app.use(NoticeBar)
app.use(DropdownMenu)
app.use(DropdownItem)
app.use(Grid)
app.use(GridItem)

app.use(createPinia())
app.use(router)

app.mount('#app')
