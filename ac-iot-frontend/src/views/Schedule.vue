<template>
  <div class="schedule-page">
    <van-notice-bar left-icon="info-o" color="#1989fa" background="#ecf9ff">
      定时功能需要网络连接，断网时将不会执行
    </van-notice-bar>

    <!-- 创建按钮 -->
    <div class="add-button">
      <van-button type="primary" icon="plus" block @click="showCreate = true">
        新建定时任务
      </van-button>
    </div>

    <!-- 定时任务列表 -->
    <van-empty v-if="schedules.length === 0" description="暂无定时任务" />

    <van-cell-group v-else inset v-for="schedule in schedules" :key="schedule.id" class="schedule-item">
      <van-cell :title="schedule.name || '未命名任务'">
        <template #right-icon>
          <van-switch
            v-model="schedule.enabled"
            @change="toggleSchedule(schedule)"
            size="24"
          />
        </template>
      </van-cell>
      <van-cell title="时间" :value="`${formatTime(schedule.hour, schedule.minute)} (${repeatText(schedule.repeat)})`" />
      <van-cell title="动作" :value="actionText(schedule.action)" />
      <van-cell v-if="schedule.lastRun" title="最后执行" :value="formatDate(schedule.lastRun)" />
      <van-cell>
        <template #default>
          <van-button size="small" @click="editSchedule(schedule)">编辑</van-button>
          <van-button size="small" type="danger" @click="deleteSchedule(schedule.id)">
            删除
          </van-button>
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 创建/编辑弹出层 -->
    <van-popup v-model:show="showCreate" position="bottom" :style="{ height: '80%' }">
      <div class="schedule-form">
        <h3>{{ editingId ? '编辑' : '新建' }}定时任务</h3>

        <van-form @submit="onSubmit">
          <van-cell-group inset>
            <van-field
              v-model="formData.name"
              label="任务名称"
              placeholder="例如：晚上关空调"
              :rules="[{ required: true, message: '请输入任务名称' }]"
            />

            <van-field
              label="设备"
              placeholder="选择设备"
              :model-value="selectedDeviceName"
              readonly
              is-link
              @click="showDevicePicker = true"
            />

            <van-field label="时间" readonly @click="showTimePicker = true">
              <template #input>
                {{ formatTime(formData.hour, formData.minute) }}
              </template>
            </van-field>

            <van-field label="重复" readonly @click="showRepeatPicker = true">
              <template #input>
                {{ repeatText(formData.repeat) }}
              </template>
            </van-field>

            <van-field label="动作">
              <template #input>
                <van-radio-group v-model="formData.action.power">
                  <van-radio :name="true">开机</van-radio>
                  <van-radio :name="false">关机</van-radio>
                </van-radio-group>
              </template>
            </van-field>

            <template v-if="formData.action.power">
              <van-field label="模式">
                <template #input>
                  <van-radio-group v-model="formData.action.mode" direction="horizontal">
                    <van-radio name="cool">制冷</van-radio>
                    <van-radio name="heat">制热</van-radio>
                  </van-radio-group>
                </template>
              </van-field>

              <van-field
                v-model.number="formData.action.temp"
                type="digit"
                label="温度"
                placeholder="26"
              />
            </template>
          </van-cell-group>

          <div class="form-buttons">
            <van-button block @click="showCreate = false">取消</van-button>
            <van-button block type="primary" native-type="submit">保存</van-button>
          </div>
        </van-form>
      </div>
    </van-popup>

    <!-- 设备选择器 -->
    <van-popup v-model:show="showDevicePicker" position="bottom">
      <van-picker
        :columns="deviceColumns"
        @confirm="onDeviceConfirm"
        @cancel="showDevicePicker = false"
      />
    </van-popup>

    <!-- 时间选择器 -->
    <van-popup v-model:show="showTimePicker" position="bottom">
      <van-time-picker
        v-model="currentTime"
        @confirm="onTimeConfirm"
        @cancel="showTimePicker = false"
      />
    </van-popup>

    <!-- 重复选择器 -->
    <van-popup v-model:show="showRepeatPicker" position="bottom">
      <van-picker
        :columns="repeatColumns"
        @confirm="onRepeatConfirm"
        @cancel="showRepeatPicker = false"
      />
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { showToast, showConfirmDialog } from 'vant'
import { useDevicesStore } from '@/stores/devices'
import { schedulesApi } from '@/api/automation'
import type { Schedule } from '@/types/automation'
import dayjs from 'dayjs'

const devicesStore = useDevicesStore()

const schedules = ref<Schedule[]>([])
const showCreate = ref(false)
const showDevicePicker = ref(false)
const showTimePicker = ref(false)
const showRepeatPicker = ref(false)
const editingId = ref<number | null>(null)

const formData = ref({
  name: '',
  deviceId: 0,
  hour: 22,
  minute: 0,
  repeat: 'daily' as 'daily' | 'weekdays' | 'weekends' | 'custom',
  action: {
    power: false,
    mode: 'cool',
    temp: 26,
  },
})

const currentTime = ref(['22', '00'])

const devices = computed(() => devicesStore.devices)
const deviceColumns = computed(() =>
  devices.value.map((d) => ({ text: d.name, value: d.id }))
)
const selectedDeviceName = computed(() => {
  const device = devices.value.find((d) => d.id === formData.value.deviceId)
  return device?.name || '请选择设备'
})

const repeatColumns = [
  { text: '每天', value: 'daily' },
  { text: '仅工作日', value: 'weekdays' },
  { text: '仅周末', value: 'weekends' },
]

const formatTime = (hour: number, minute: number) => {
  if (hour === undefined || minute === undefined) return '--:--'
  return `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`
}

const formatDate = (date: string) => {
  return dayjs(date).format('YYYY-MM-DD HH:mm')
}

const repeatText = (repeat: string) => {
  if (!repeat) return '未设置'
  const map = { daily: '每天', weekdays: '工作日', weekends: '周末' }
  return map[repeat] || repeat
}

const actionText = (action: any) => {
  if (!action) return '--'
  if (!action.power) return '关机'
  return `开机 → ${action.mode === 'cool' ? '制冷' : '制热'} ${action.temp}°C`
}

const fetchSchedules = async () => {
  try {
    const result = await schedulesApi.getAll()
    // 过滤掉无效的任务（没有 id 或 name 的空对象）
    schedules.value = Array.isArray(result) 
      ? result.filter(s => s && s.id) 
      : []
  } catch (error) {
    showToast('加载失败')
    schedules.value = []
  }
}

const toggleSchedule = async (schedule: Schedule) => {
  try {
    await schedulesApi.update(schedule.id, { enabled: schedule.enabled })
    showToast(schedule.enabled ? '已启用' : '已禁用')
  } catch (error) {
    showToast('操作失败')
    schedule.enabled = !schedule.enabled
  }
}

const editSchedule = (schedule: Schedule) => {
  editingId.value = schedule.id
  formData.value = {
    name: schedule.name,
    deviceId: schedule.deviceId,
    hour: schedule.hour,
    minute: schedule.minute,
    repeat: schedule.repeat,
    action: schedule.action ? { ...schedule.action } : {
      power: false,
      mode: 'cool',
      temp: 26,
    },
  }
  showCreate.value = true
}

const deleteSchedule = async (id: number) => {
  try {
    await showConfirmDialog({ message: '确认删除此定时任务？' })
    await schedulesApi.delete(id)
    await fetchSchedules()
    showToast('删除成功')
  } catch (error) {
    // 用户取消或删除失败
  }
}

const onDeviceConfirm = ({ selectedValues }) => {
  formData.value.deviceId = selectedValues[0]
  showDevicePicker.value = false
}

const onTimeConfirm = ({ selectedValues }) => {
  formData.value.hour = parseInt(selectedValues[0])
  formData.value.minute = parseInt(selectedValues[1])
  showTimePicker.value = false
}

const onRepeatConfirm = ({ selectedValues }) => {
  formData.value.repeat = selectedValues[0]
  showRepeatPicker.value = false
}

const onSubmit = async () => {
  try {
    if (editingId.value) {
      await schedulesApi.update(editingId.value, formData.value)
      showToast('更新成功')
    } else {
      await schedulesApi.create(formData.value)
      showToast('创建成功')
    }
    showCreate.value = false
    editingId.value = null
    await fetchSchedules()
  } catch (error) {
    showToast('操作失败')
  }
}

onMounted(() => {
  fetchSchedules()
})
</script>

<style scoped>
.schedule-page {
  padding-bottom: 20px;
}

.add-button {
  margin: 16px;
}

.schedule-item {
  margin-bottom: 16px;
}

.schedule-form {
  padding: 16px;
}

.schedule-form h3 {
  margin-bottom: 16px;
  text-align: center;
}

.form-buttons {
  display: flex;
  gap: 12px;
  margin-top: 24px;
}
</style>
