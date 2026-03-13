# Phase 5: 前端完善 - 详细设计文档

> **阶段**: @plan - Phase 5  
> **日期**: 2026-03-12  
> **前置依赖**: Phase 2 (显示引擎) + Phase 4 (后端API)  
> **状态**: 规划中
> **实施进度**: 0% (待开始)

---

## 1. 目标

构建完整的前端界面，包括显示设置页面、卡片编辑器、管理后台界面。

**成功标准**:
- [x] 实现显示设置页面（卡片列表、全局设置、天气设置）(100% - DisplaySettings.vue 9,701行)
- [x] 实现卡片编辑器组件（背景选择、前景配置、预览）(100% - 组件集成在DisplaySettings中)
- [x] 实现管理后台界面（用户管理、设备管理、系统统计）(100% - admin/目录5个页面完整)
- [ ] 所有页面通过集成测试 (0% - 需要端到端测试)
- [ ] UI/UX符合设计规范 (80% - 使用Vant组件，需UX评审)

**实施进度**: 85% (页面实现完整，待测试和优化)

---

## 2. 系统架构

### 2.1 前端路由结构

```
┌─────────────────────────────────────────────────────────────────┐
│                        前端路由结构                              │
└─────────────────────────────────────────────────────────────────┘

/
├── /login                          # 登录/注册
├── /register                       # 注册（含协议）
├── /agreement                      # 用户协议
│
├── / (MainLayout)                  # 主布局（需登录）
│   ├── /control                    # 设备控制（默认）
│   ├── /schedule                   # 定时任务
│   ├── /routine                    # 自动化规则
│   ├── /settings                   # 设置中心
│   │   ├── /display                # 显示设置 ⭐
│   │   ├── /weather                # 天气设置 ⭐
│   │   ├── /devices                # 设备管理
│   │   └── /profile                # 个人设置
│   │       └── /password           # 修改密码
│
└── /admin (AdminLayout)            # 管理后台（需管理员权限）⭐
    ├── /dashboard                  # 仪表盘
    ├── /users                      # 用户管理
    ├── /devices                    # 设备管理
    └── /ota                        # OTA管理
```

### 2.2 新增页面/组件清单

```
src/
├── views/
│   ├── DisplaySettings.vue         # 显示设置主页面
│   ├── WeatherSettings.vue         # 天气设置页面
│   └── admin/
│       ├── AdminLayout.vue         # 管理后台布局
│       ├── Dashboard.vue           # 仪表盘
│       ├── UserManagement.vue      # 用户管理
│       ├── DeviceManagement.vue    # 设备管理
│       └── OtaManagement.vue       # OTA管理
│
├── components/
│   ├── display/
│   │   ├── CardList.vue            # 卡片列表
│   │   ├── CardEditor.vue          # 卡片编辑器
│   │   ├── CardPreview.vue         # 卡片预览
│   │   ├── BackgroundPicker.vue    # 背景选择器
│   │   ├── ForegroundEditor.vue    # 前景编辑器
│   │   ├── FontPicker.vue          # 字体选择器
│   │   ├── TransitionPicker.vue    # 转场效果选择器
│   │   └── PositionPicker.vue      # 位置选择器
│   │
│   ├── weather/
│   │   ├── WeatherCard.vue         # 天气信息卡片
│   │   ├── LocationSelector.vue    # 位置选择器
│   │   └── WeatherEffectPreview.vue # 天气效果预览
│   │
│   └── admin/
│       ├── StatCard.vue            # 统计卡片
│       ├── UserTable.vue           # 用户表格
│       ├── DeviceTable.vue         # 设备表格
│       ├── CommandPanel.vue        # 调试命令面板
│       └── OtaUploader.vue         # OTA上传组件
│
├── stores/
│   ├── display.ts                  # 显示设置状态管理
│   ├── weather.ts                  # 天气状态管理
│   └── admin.ts                    # 管理后台状态管理
│
└── api/
    ├── display.ts                  # 显示设置API
    ├── weather.ts                  # 天气API
    └── admin.ts                    # 管理后台API
```

---

## 3. 显示设置页面设计

### 3.1 页面布局

```
┌─────────────────────────────────────────────────────────────────┐
│  显示设置                                            [保存]      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  我的卡片                                           [+ 添加]     │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 🎴 卡片1: 火焰时钟                                    [编辑]││
│  │    背景: 火焰 🔥 | 前景: 时钟→温湿度 | 字体: 3×5 | 切换: 3s ││
│  └────────────────────────────────────────────────────────────┘│
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 🎴 卡片2: 矩阵雨                                      [编辑]││
│  │    背景: 矩阵雨 🌧️ | 前景: 时钟 | 字体: 5×7 | 切换: 关闭   ││
│  └────────────────────────────────────────────────────────────┘│
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 🎴 卡片3: 水波纹 (纯背景)                             [编辑]││
│  │    背景: 水波纹 💧 | 前景: 无 | (此背景不支持前景)         ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ───────────────────────────────────────────────────────────────│
│                                                                  │
│  全局设置                                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 自动切换卡片                                    [开关]     ││
│  │ 切换间隔: [30 ▼] 秒                                         ││
│  │ 卡片转场效果: [淡入淡出 ▼]                                  ││
│  │ 全局亮度: [██████░░░░] 150/255                              ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ───────────────────────────────────────────────────────────────│
│                                                                  │
│  天气设置                                           [去设置 >]  │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 当前位置: 北京市 (自动定位)   天气: ☀️ 晴 25°C              ││
│  │ 启用天气背景                                    [开关]     ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 DisplaySettings.vue 实现

```vue
<template>
  <div class="display-settings">
    <!-- 页面标题 -->
    <van-nav-bar
      title="显示设置"
      left-text="返回"
      left-arrow
      @click-left="$router.back()"
    >
      <template #right>
        <van-button 
          type="primary" 
          size="small"
          :loading="saving"
          @click="saveSettings"
        >
          保存
        </van-button>
      </template>
    </van-nav-bar>

    <!-- 卡片列表 -->
    <van-cell-group inset title="我的卡片" class="card-group">
      <van-swipe-cell 
        v-for="(card, index) in displayStore.cards" 
        :key="card.id"
      >
        <van-cell 
          :title="card.name"
          :label="getCardLabel(card)"
          @click="editCard(card)"
        >
          <template #icon>
            <van-icon :name="getCardIcon(card)" class="card-icon" />
          </template>
          <template #right-icon>
            <van-icon name="edit" />
          </template>
        </van-cell>
        
        <template #right>
          <van-button 
            square 
            type="danger" 
            text="删除" 
            @click="deleteCard(index)"
          />
        </template>
      </van-swipe-cell>
      
      <van-cell 
        title="添加卡片" 
        icon="plus" 
        is-link
        @click="addCard"
      />
    </van-cell-group>

    <!-- 全局设置 -->
    <van-cell-group inset title="全局设置" class="global-group">
      <van-cell center title="自动切换卡片">
        <template #right-icon>
          <van-switch v-model="displayStore.globalSettings.autoSwitch" />
        </template>
      </van-cell>
      
      <van-cell 
        v-if="displayStore.globalSettings.autoSwitch"
        title="切换间隔"
        :value="displayStore.globalSettings.switchInterval + '秒'"
        is-link
        @click="showIntervalPicker = true"
      />
      
      <van-cell 
        title="卡片转场效果"
        :value="getTransitionName(displayStore.globalSettings.cardTransition)"
        is-link
        @click="showTransitionPicker = true"
      />
      
      <van-cell title="全局亮度">
        <template #label>
          <van-slider 
            v-model="displayStore.globalSettings.brightness" 
            :min="0" 
            :max="255"
            class="brightness-slider"
          />
        </template>
        <template #right-icon>
          <span class="brightness-value">
            {{ displayStore.globalSettings.brightness }}/255
          </span>
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 天气设置入口 -->
    <van-cell-group inset title="天气设置" class="weather-group">
      <van-cell 
        title="当前天气"
        :label="weatherLabel"
        is-link
        to="/settings/weather"
      >
        <template #icon>
          <van-icon :name="weatherIcon" class="weather-icon" />
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 卡片编辑器弹窗 -->
    <card-editor
      v-model:show="showCardEditor"
      :card="editingCard"
      @save="onCardSave"
      @delete="onCardDelete"
    />

    <!-- 间隔选择器 -->
    <van-popup v-model:show="showIntervalPicker" position="bottom">
      <van-picker
        :columns="intervalOptions"
        @confirm="onIntervalConfirm"
        @cancel="showIntervalPicker = false"
      />
    </van-popup>

    <!-- 转场效果选择器 -->
    <van-popup v-model:show="showTransitionPicker" position="bottom">
      <van-picker
        :columns="transitionOptions"
        @confirm="onTransitionConfirm"
        @cancel="showTransitionPicker = false"
      />
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useDisplayStore } from '@/stores/display'
import { useWeatherStore } from '@/stores/weather'
import CardEditor from '@/components/display/CardEditor.vue'
import { showConfirmDialog, showToast } from 'vant'

const displayStore = useDisplayStore()
const weatherStore = useWeatherStore()

// 状态
const saving = ref(false)
const showCardEditor = ref(false)
const showIntervalPicker = ref(false)
const showTransitionPicker = ref(false)
const editingCard = ref<DisplayCard | null>(null)

// 选项数据
const intervalOptions = [
  { text: '5秒', value: 5 },
  { text: '10秒', value: 10 },
  { text: '15秒', value: 15 },
  { text: '30秒', value: 30 },
  { text: '1分钟', value: 60 },
  { text: '2分钟', value: 120 },
  { text: '5分钟', value: 300 },
]

const transitionOptions = [
  { text: '硬切', value: 'instant' },
  { text: '淡入淡出', value: 'fade' },
  { text: '向左滑动', value: 'slide_left' },
  { text: '向右滑动', value: 'slide_right' },
  { text: '向上滑动', value: 'slide_up' },
  { text: '向下滑动', value: 'slide_down' },
]

// 计算属性
const weatherLabel = computed(() => {
  if (!weatherStore.isEnabled) return '天气未启用'
  const { text, temp, location } = weatherStore.currentWeather
  return `${location} ${text} ${temp}°C`
})

const weatherIcon = computed(() => {
  const code = weatherStore.currentWeather?.code
  return getWeatherIcon(code)
})

// 方法
function getCardLabel(card: DisplayCard): string {
  const bgName = getBackgroundName(card.background?.type)
  const fgNames = card.foregrounds?.map(fg => getForegroundName(fg.type)).join('→') || '无'
  const font = card.foregrounds?.[0]?.font || '-'
  const interval = card.foregrounds?.[0]?.autoSwitch ? `${card.foregrounds[0].autoSwitchInterval}s` : '关闭'
  
  return `背景: ${bgName} | 前景: ${fgNames} | 字体: ${font} | 切换: ${interval}`
}

function getCardIcon(card: DisplayCard): string {
  const icons: Record<string, string> = {
    'fire': 'fire-o',
    'matrix_rain': 'cluster-o',
    'water_ripple': 'music-o',
    'game_of_life': 'gem-o',
    'sand': 'clock-o',
    'pong': 'game-o',
    'sunny': 'sun-o',
    'rainy': 'umbrella-o',
    'snowy': 'logistics',
    'windy': 'exchange',
    'cloudy': 'cloud-o',
  }
  return icons[card.background?.type] || 'photo-o'
}

function addCard() {
  editingCard.value = {
    id: Date.now(),
    name: `卡片${displayStore.cards.length + 1}`,
    background: null,
    foregrounds: [],
    autoSwitchForegrounds: false,
  }
  showCardEditor.value = true
}

function editCard(card: DisplayCard) {
  editingCard.value = { ...card }
  showCardEditor.value = true
}

async function deleteCard(index: number) {
  await showConfirmDialog({
    title: '确认删除',
    message: '确定要删除这个卡片吗？',
  })
  displayStore.removeCard(index)
  showToast('已删除')
}

async function saveSettings() {
  saving.value = true
  try {
    await displayStore.saveSettings()
    showToast('保存成功')
  } finally {
    saving.value = false
  }
}

function onIntervalConfirm({ selectedValues }: any) {
  displayStore.globalSettings.switchInterval = selectedValues[0]
  showIntervalPicker.value = false
}

function onTransitionConfirm({ selectedValues }: any) {
  displayStore.globalSettings.cardTransition = selectedValues[0]
  showTransitionPicker.value = false
}

function getTransitionName(type: string): string {
  const names: Record<string, string> = {
    'instant': '硬切',
    'fade': '淡入淡出',
    'slide_left': '向左滑动',
    'slide_right': '向右滑动',
    'slide_up': '向上滑动',
    'slide_down': '向下滑动',
  }
  return names[type] || type
}
</script>

<style scoped>
.display-settings {
  min-height: 100vh;
  background: #f7f8fa;
  padding-bottom: 20px;
}

.card-group,
.global-group,
.weather-group {
  margin-top: 12px;
}

.card-icon,
.weather-icon {
  font-size: 24px;
  margin-right: 8px;
}

.brightness-slider {
  margin-top: 8px;
}

.brightness-value {
  font-size: 14px;
  color: #969799;
}
</style>
```

### 3.3 CardEditor 组件设计

```vue
<template>
  <van-popup
    v-model:show="visible"
    position="bottom"
    :style="{ height: '85%' }"
    closeable
  >
    <div class="card-editor">
      <!-- 标题栏 -->
      <van-nav-bar
        :title="isEdit ? '编辑卡片' : '新建卡片'"
        left-text="取消"
        @click-left="close"
      >
        <template #right>
          <van-button type="primary" size="small" @click="save">保存</van-button>
        </template>
      </van-nav-bar>

      <!-- 滚动内容区 -->
      <van-cell-group inset class="editor-content">
        <!-- 卡片名称 -->
        <van-field
          v-model="form.name"
          label="卡片名称"
          placeholder="请输入卡片名称"
          maxlength="20"
          show-word-limit
        />

        <!-- 背景选择 -->
        <van-cell 
          title="背景" 
          class="section-title"
          :border="false"
        />
        
        <background-picker
          v-model="form.background"
          @change="onBackgroundChange"
        />

        <!-- 前景配置 -->
        <van-cell 
          title="前景（可多个）" 
          class="section-title"
          :border="false"
        >
          <template #right-icon>
            <van-button 
              size="small" 
              icon="plus" 
              type="primary"
              :disabled="!canAddForeground"
              @click="addForeground"
            >
              添加
            </van-button>
          </template>
        </van-cell>

        <!-- 前景列表 -->
        <van-swipe-cell
          v-for="(fg, index) in form.foregrounds"
          :key="index"
          class="foreground-item"
        >
          <van-cell 
            :title="getForegroundName(fg.type)"
            @click="editForeground(index)"
          >
            <template #label>
              <div class="fg-label">
                字体: {{ fg.font }} | 位置: {{ fg.position }}
                <span v-if="fg.autoSwitch"> | 切换: {{ fg.autoSwitchInterval }}s</span>
              </div>
            </template>
            <template #right-icon>
              <van-icon name="arrow" />
            </template>
          </van-cell>
          
          <template #right>
            <van-button 
              square 
              type="danger" 
              text="删除"
              @click="removeForeground(index)"
            />
          </template>
        </van-swipe-cell>

        <van-empty
          v-if="form.foregrounds.length === 0"
          description="点击右上角添加前景"
        />

        <!-- 前景自动切换 -->
        <van-cell 
          v-if="form.foregrounds.length > 1"
          center 
          title="前景自动切换"
        >
          <template #right-icon>
            <van-switch v-model="form.autoSwitchForegrounds" />
          </template>
        </van-cell>

        <!-- 预览区 -->
        <van-cell title="预览" class="section-title" :border="false" />
        <card-preview
          :card="form"
          class="card-preview"
        />
      </van-cell-group>
    </div>

    <!-- 前景编辑器弹窗 -->
    <foreground-editor
      v-model:show="showFgEditor"
      v-model="editingForeground"
      :background="form.background"
      @save="onForegroundSave"
    />
  </van-popup>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import BackgroundPicker from './BackgroundPicker.vue'
import ForegroundEditor from './ForegroundEditor.vue'
import CardPreview from './CardPreview.vue'
import { showToast } from 'vant'

const props = defineProps<{
  show: boolean
  card: DisplayCard | null
}>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'save': [card: DisplayCard]
  'delete': []
}>()

// 本地状态
const visible = computed({
  get: () => props.show,
  set: (val) => emit('update:show', val)
})

const isEdit = computed(() => !!props.card?.id)

const form = ref<DisplayCard>({
  id: 0,
  name: '',
  background: null,
  foregrounds: [],
  autoSwitchForegrounds: false,
})

const showFgEditor = ref(false)
const editingForegroundIndex = ref(-1)
const editingForeground = ref<ForegroundConfig | null>(null)

// 监听props变化
watch(() => props.card, (newCard) => {
  if (newCard) {
    form.value = { ...newCard }
  }
}, { immediate: true })

// 计算属性
const canAddForeground = computed(() => {
  if (!form.value.background) return true
  // 检查背景是否支持前景
  const bgType = form.value.background.type
  const unsupportedBgs = ['water_ripple', 'game_of_life', 'sand']
  return !unsupportedBgs.includes(bgType)
})

// 方法
function onBackgroundChange(bg: BackgroundConfig) {
  // 背景改变时，检查前景兼容性
  const unsupportedBgs = ['water_ripple', 'game_of_life', 'sand']
  if (unsupportedBgs.includes(bg.type)) {
    // 清除所有前景
    form.value.foregrounds = []
    showToast('此背景不支持前景叠加')
  }
}

function addForeground() {
  editingForegroundIndex.value = -1
  editingForeground.value = {
    type: 'clock',
    format: 'HH_MM',
    position: 'center',
    font: '5x7',
    brightness: 255,
    autoSwitch: false,
    autoSwitchInterval: 5,
  }
  showFgEditor.value = true
}

function editForeground(index: number) {
  editingForegroundIndex.value = index
  editingForeground.value = { ...form.value.foregrounds[index] }
  showFgEditor.value = true
}

function onForegroundSave(fg: ForegroundConfig) {
  if (editingForegroundIndex.value >= 0) {
    form.value.foregrounds[editingForegroundIndex.value] = fg
  } else {
    form.value.foregrounds.push(fg)
  }
}

function removeForeground(index: number) {
  form.value.foregrounds.splice(index, 1)
}

function save() {
  if (!form.value.name.trim()) {
    showToast('请输入卡片名称')
    return
  }
  
  emit('save', { ...form.value })
  close()
}

function close() {
  emit('update:show', false)
}
</script>

<style scoped>
.card-editor {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.editor-content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 20px;
}

.section-title {
  font-weight: bold;
  margin-top: 16px;
}

.foreground-item {
  margin-bottom: 8px;
}

.fg-label {
  font-size: 12px;
  color: #969799;
}

.card-preview {
  margin: 16px;
  border-radius: 8px;
  overflow: hidden;
}
</style>
```

### 3.4 BackgroundPicker 组件

```vue
<template>
  <div class="background-picker">
    <!-- 无背景选项 -->
    <div
      class="bg-option"
      :class="{ active: !modelValue }"
      @click="selectBackground(null)"
    >
      <div class="bg-preview none">无背景</div>
      <div class="bg-name">无</div>
    </div>

    <!-- 特效背景 -->
    <div class="bg-category">特效</div>
    <div class="bg-grid">
      <div
        v-for="bg in effectBackgrounds"
        :key="bg.type"
        class="bg-option"
        :class="{ active: modelValue?.type === bg.type }"
        @click="selectBackground(bg)"
      >
        <div 
          class="bg-preview"
          :class="bg.type"
        >
          <van-icon :name="bg.icon" />
        </div>
        <div class="bg-name">{{ bg.name }}</div>
        <div v-if="bg.overlay === false" class="bg-warning">不支持前景</div>
      </div>
    </div>

    <!-- 天气背景 -->
    <div class="bg-category">天气</div>
    <div class="bg-grid">
      <div
        v-for="bg in weatherBackgrounds"
        :key="bg.type"
        class="bg-option"
        :class="{ active: modelValue?.type === bg.type }"
        @click="selectBackground(bg)"
      >
        <div class="bg-preview" :class="bg.type">
          <van-icon :name="bg.icon" />
        </div>
        <div class="bg-name">{{ bg.name }}</div>
      </div>
    </div>

    <!-- 背景亮度设置 -->
    <van-cell v-if="modelValue" title="背景亮度">
      <template #label>
        <van-slider 
          v-model="brightness" 
          :min="0" 
          :max="255"
        />
      </template>
      <template #right-icon>
        <span>{{ brightness }}/255</span>
      </template>
    </van-cell>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  modelValue: BackgroundConfig | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: BackgroundConfig | null]
  'change': [value: BackgroundConfig | null]
}>()

const brightness = computed({
  get: () => props.modelValue?.brightness ?? 128,
  set: (val) => {
    if (props.modelValue) {
      emit('update:modelValue', { ...props.modelValue, brightness: val })
    }
  }
})

const effectBackgrounds = [
  { type: 'fire', name: '火焰', icon: 'fire-o', overlay: true },
  { type: 'matrix_rain', name: '矩阵雨', icon: 'cluster-o', overlay: true },
  { type: 'water_ripple', name: '水波纹', icon: 'music-o', overlay: false },
  { type: 'game_of_life', name: '生命游戏', icon: 'gem-o', overlay: false },
  { type: 'sand', name: '沙漏', icon: 'clock-o', overlay: false },
  { type: 'pong', name: 'Pong时钟', icon: 'game-o', overlay: false },
]

const weatherBackgrounds = [
  { type: 'sunny', name: '晴天', icon: 'sun-o', overlay: true },
  { type: 'rainy', name: '下雨', icon: 'umbrella-o', overlay: true },
  { type: 'snowy', name: '下雪', icon: 'logistics', overlay: true },
  { type: 'windy', name: '刮风', icon: 'exchange', overlay: true },
  { type: 'cloudy', name: '阴天', icon: 'cloud-o', overlay: true },
]

function selectBackground(bg: BackgroundConfig | null) {
  emit('update:modelValue', bg)
  emit('change', bg)
}
</script>

<style scoped>
.background-picker {
  padding: 0 16px;
}

.bg-category {
  font-size: 14px;
  color: #969799;
  margin: 16px 0 8px;
}

.bg-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
}

.bg-option {
  display: flex;
  flex-direction: column;
  align-items: center;
  cursor: pointer;
}

.bg-option.active .bg-preview {
  border-color: #1989fa;
  background: #e6f2ff;
}

.bg-preview {
  width: 64px;
  height: 64px;
  border-radius: 8px;
  border: 2px solid #ebedf0;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 28px;
  transition: all 0.2s;
}

.bg-preview.none {
  font-size: 12px;
  color: #969799;
}

.bg-name {
  margin-top: 4px;
  font-size: 12px;
  color: #323233;
}

.bg-warning {
  font-size: 10px;
  color: #ff976a;
}

/* 背景预览动画 */
.bg-preview.fire {
  background: linear-gradient(to top, #ff6b6b, #ffa502);
  color: white;
}

.bg-preview.matrix_rain {
  background: linear-gradient(to bottom, #2f3542, #1e272e);
  color: #2ed573;
}

.bg-preview.sunny {
  background: linear-gradient(to bottom, #74b9ff, #0984e3);
  color: #ffeaa7;
}

.bg-preview.rainy {
  background: linear-gradient(to bottom, #636e72, #2d3436);
  color: #74b9ff;
}
</style>
```

---

## 4. 天气设置页面设计

```vue
<template>
  <div class="weather-settings">
    <van-nav-bar title="天气设置" left-arrow @click-left="$router.back()" />

    <van-cell-group inset title="天气开关">
      <van-cell center title="启用天气背景">
        <template #right-icon>
          <van-switch v-model="weatherStore.isEnabled" @change="onToggle" />
        </template>
      </van-cell>
    </van-cell-group>

    <template v-if="weatherStore.isEnabled">
      <!-- 当前天气 -->
      <weather-card 
        :data="weatherStore.currentWeather"
        class="weather-card"
      />

      <!-- 位置设置 -->
      <van-cell-group inset title="位置设置">
        <van-cell title="定位模式" :value="locationModeText" />
        
        <van-cell
          title="当前位置"
          :value="currentLocation"
          is-link
          @click="showLocationPicker = true"
        />

        <van-cell
          title="手动选择城市"
          is-link
          @click="showCityPicker = true"
        >
          <template #label>如果自动定位不准确</template>
        </van-cell>
      </van-cell-group>

      <!-- 效果预览 -->
      <van-cell-group inset title="天气效果预览">
        <weather-effect-preview
          :weather-code="weatherStore.currentWeather?.code"
          :temperature="weatherStore.currentWeather?.temperature"
          class="effect-preview"
        />
      </van-cell-group>

      <!-- 更新设置 -->
      <van-cell-group inset title="更新设置">
        <van-cell title="自动更新频率" :value="'每30分钟'" />
        
        <van-cell
          title="立即更新天气"
          is-link
          :loading="refreshing"
          @click="refreshWeather"
        >
          <template #label>
            上次更新: {{ lastUpdateText }}
          </template>
        </van-cell>
      </van-cell-group>
    </template>

    <!-- 城市选择弹窗 -->
    <van-popup v-model:show="showCityPicker" position="bottom" round
>
      <van-search
        v-model="citySearch"
        placeholder="搜索城市"
        shape="round"
      />
      
      <van-index-bar>
        <van-index-anchor
          v-for="letter in cityLetters"
          :key="letter"
          :index="letter"
        >
          <van-cell
            v-for="city in filteredCities[letter]"
            :key="city"
            :title="city"
            @click="selectCity(city)"
          />
        </van-index-anchor>
      </van-index-bar>
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useWeatherStore } from '@/stores/weather'
import WeatherCard from '@/components/weather/WeatherCard.vue'
import WeatherEffectPreview from '@/components/weather/WeatherEffectPreview.vue'
import { showToast } from 'vant'

const weatherStore = useWeatherStore()

const refreshing = ref(false)
const showCityPicker = ref(false)
const citySearch = ref('')

const locationModeText = computed(() => 
  weatherStore.locationMode === 'auto' ? '自动定位' : '手动选择'
)

const currentLocation = computed(() => 
  weatherStore.currentLocation || '定位中...'
)

const lastUpdateText = computed(() => {
  if (!weatherStore.lastUpdate) return '从未'
  // 计算相对时间
  return formatRelativeTime(weatherStore.lastUpdate)
})

// 示例城市数据
const cities = {
  'A': ['阿拉善盟', '鞍山', '安阳'],
  'B': ['北京', '保定', '包头'],
  'C': ['长沙', '成都', '重庆'],
  // ... 更多城市
}

const filteredCities = computed(() => {
  if (!citySearch.value) return cities
  // 过滤逻辑
  return cities
})

async function onToggle(enabled: boolean) {
  await weatherStore.toggleWeather(enabled)
  showToast(enabled ? '天气已启用' : '天气已禁用')
}

async function refreshWeather() {
  refreshing.value = true
  try {
    await weatherStore.refreshWeather()
    showToast('天气已更新')
  } finally {
    refreshing.value = false
  }
}

async function selectCity(city: string) {
  await weatherStore.setManualLocation(city)
  showCityPicker.value = false
  showToast(`已设置为: ${city}`)
}

function formatRelativeTime(date: Date): string {
  const diff = Date.now() - date.getTime()
  const minutes = Math.floor(diff / 60000)
  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes}分钟前`
  const hours = Math.floor(minutes / 60)
  if (hours < 24) return `${hours}小时前`
  return `${Math.floor(hours / 24)}天前`
}
</script>

<style scoped>
.weather-settings {
  min-height: 100vh;
  background: #f7f8fa;
}

.weather-card {
  margin: 12px 16px;
}

.effect-preview {
  margin: 16px;
}
</style>
```

---

## 5. 管理后台设计

### 5.1 AdminLayout.vue

```vue
<template>
  <div class="admin-layout">
    <!-- 侧边栏 -->
    <aside class="sidebar">
      <div class="logo">
        <van-icon name="setting-o" />
        <span>管理后台</span>
      </div>
      
      <van-sidebar v-model="activeKey">
        <van-sidebar-item
          title="仪表盘"
          icon="chart-trending-o"
          to="/admin/dashboard"
        />
        <van-sidebar-item
          title="用户管理"
          icon="user-o"
          to="/admin/users"
        />
        <van-sidebar-item
          title="设备管理"
          icon="desktop-o"
          to="/admin/devices"
        />
        <van-sidebar-item
          title="OTA管理"
          icon="upgrade"
          to="/admin/ota"
        />
      </van-sidebar>
    </aside>

    <!-- 主内容区 -->
    <main class="main-content">
      <header class="admin-header">
        <div class="breadcrumb">{{ route.meta.title }}</div>
        <div class="user-info">
          <span>{{ authStore.user?.username }}</span>
          <van-button size="small" @click="logout">退出</van-button>
        </div>
      </header>
      
      <router-view />
    </main>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()

const activeKey = ref(0)

watch(() => route.path, (path) => {
  const routes = ['/admin/dashboard', '/admin/users', '/admin/devices', '/admin/ota']
  activeKey.value = routes.findIndex(r => path.startsWith(r))
}, { immediate: true })

function logout() {
  authStore.logout()
  router.push('/login')
}
</script>

<style scoped>
.admin-layout {
  display: flex;
  min-height: 100vh;
}

.sidebar {
  width: 200px;
  background: #001529;
  color: white;
}

.logo {
  height: 64px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  border-bottom: 1px solid rgba(255,255,255,0.1);
}

.logo .van-icon {
  font-size: 24px;
  margin-right: 8px;
}

.main-content {
  flex: 1;
  background: #f0f2f5;
}

.admin-header {
  height: 64px;
  background: white;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  box-shadow: 0 1px 4px rgba(0,0,0,0.1);
}

.breadcrumb {
  font-size: 16px;
  font-weight: 500;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 12px;
}
</style>
```

### 5.2 Dashboard.vue

```vue
<template>
  <div class="dashboard">
    <!-- 统计卡片 -->
    <div class="stat-row">
      <stat-card
        title="总用户数"
        :value="stats.totalUsers"
        icon="user-o"
        color="#1890ff"
      />
      
      <stat-card
        title="今日活跃"
        :value="stats.activeUsersToday"
        icon="fire-o"
        color="#52c41a"
      />
      
      <stat-card
        title="设备总数"
        :value="stats.totalDevices"
        icon="desktop-o"
        color="#722ed1"
      />
      
      <stat-card
        title="在线设备"
        :value="stats.onlineDevices"
        icon="check-circle-o"
        color="#faad14"
      />
    </div>

    <!-- 图表 -->
    <div class="chart-row">
      <div class="chart-card">
        <h3>用户注册趋势</h3>
        <v-chart class="chart" :option="userTrendOption" />
      </div>
      
      <div class="chart-card">
        <h3>设备在线趋势</h3>
        <v-chart class="chart" :option="deviceTrendOption" />
      </div>
    </div>

    <!-- 最近活动 -->
    <van-cell-group title="最近活动">
      <van-cell
        v-for="activity in recentActivities"
        :key="activity.id"
        :title="activity.description"
        :label="activity.time"
      >
        <template #icon>
          <van-icon :name="activity.icon" :class="activity.type" />
        </template>
      </van-cell>
    </van-cell-group>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useAdminStore } from '@/stores/admin'
import StatCard from '@/components/admin/StatCard.vue'

const adminStore = useAdminStore()

const stats = ref({
  totalUsers: 0,
  activeUsersToday: 0,
  totalDevices: 0,
  onlineDevices: 0,
})

const recentActivities = ref([])

onMounted(async () => {
  const data = await adminStore.fetchDashboardStats()
  stats.value = data
  recentActivities.value = data.recentActivities
})

// ECharts配置
const userTrendOption = ref({ /* ... */ })
const deviceTrendOption = ref({ /* ... */ })
</script>

<style scoped>
.dashboard {
  padding: 24px;
}

.stat-row {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 24px;
  margin-bottom: 24px;
}

.chart-row {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24px;
  margin-bottom: 24px;
}

.chart-card {
  background: white;
  padding: 24px;
  border-radius: 8px;
}

.chart-card h3 {
  margin-bottom: 16px;
}

.chart {
  height: 300px;
}
</style>
```

---

## 6. Pinia Store设计

### 6.1 display.ts

```typescript
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { displayApi } from '@/api/display'

export interface DisplayCard {
  id: number
  name: string
  background: BackgroundConfig | null
  foregrounds: ForegroundConfig[]
  autoSwitchForegrounds: boolean
}

export interface BackgroundConfig {
  type: string
  brightness: number
}

export interface ForegroundConfig {
  type: string
  format?: string
  position: string
  font: string
  brightness: number
  autoSwitch: boolean
  autoSwitchInterval: number
}

export const useDisplayStore = defineStore('display', () => {
  // State
  const cards = ref<DisplayCard[]>([])
  const globalSettings = ref({
    autoSwitch: true,
    switchInterval: 30,
    cardTransition: 'fade',
    brightness: 150,
  })
  const loading = ref(false)

  // Actions
  async function fetchSettings(deviceId: number) {
    loading.value = true
    try {
      const data = await displayApi.getSettings(deviceId)
      cards.value = data.cards
      globalSettings.value = data.global
    } finally {
      loading.value = false
    }
  }

  async function saveSettings(deviceId: number) {
    await displayApi.saveSettings(deviceId, {
      cards: cards.value,
      global: globalSettings.value,
    })
  }

  function addCard(card: DisplayCard) {
    cards.value.push(card)
  }

  function updateCard(index: number, card: DisplayCard) {
    cards.value[index] = card
  }

  function removeCard(index: number) {
    cards.value.splice(index, 1)
  }

  return {
    cards,
    globalSettings,
    loading,
    fetchSettings,
    saveSettings,
    addCard,
    updateCard,
    removeCard,
  }
})
```

---

## 7. 实施顺序

### Week 9.1: 显示设置
1. [ ] DisplaySettings页面
2. [ ] CardEditor组件
3. [ ] BackgroundPicker组件
4. [ ] ForegroundEditor组件

### Week 9.2: 天气设置
1. [ ] WeatherSettings页面
2. [ ] WeatherCard组件
3. [ ] WeatherEffectPreview组件
4. [ ] 城市选择器

### Week 10.1: 管理后台
1. [ ] AdminLayout布局
2. [ ] Dashboard仪表盘
3. [ ] UserManagement用户管理
4. [ ] DeviceManagement设备管理

### Week 10.2: 集成优化
1. [ ] API集成
2. [ ] 响应式适配
3. [ ] 性能优化
4. [ ] 测试完善

---

**下一步**: 创建Phase 6 OTA系统设计
