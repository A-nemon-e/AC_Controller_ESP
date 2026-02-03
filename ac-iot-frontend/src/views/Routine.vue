<template>
  <div class="routine-page">
    <div class="add-button">
      <van-button type="primary" icon="plus" block @click="showCreate = true">
        新建自动化
      </van-button>
    </div>

    <van-empty v-if="routines.length === 0" description="暂无自动化规则" />

    <van-cell-group
      v-else
      inset
      v-for="routine in routines"
      :key="routine.id"
      class="routine-item"
    >
      <van-cell :title="routine.name">
        <template #right-icon>
          <van-switch
            v-model="routine.enabled"
            @change="toggleRoutine(routine)"
            size="24"
          />
        </template>
      </van-cell>
      <van-cell title="逻辑" :value="routine.logic === 'AND' ? '全部满足' : '任一满足'" />
      <van-cell title="触发器" :value="`${routine.triggers.length} 个条件`" />
      <van-cell title="动作" :value="actionText(routine.action)" />
      <van-cell
        v-if="routine.lastTriggered"
        title="最后触发"
        :value="formatDate(routine.lastTriggered)"
      />
      <van-cell>
        <template #default>
          <van-button size="small" @click="editRoutine(routine)">编辑</van-button>
          <van-button size="small" type="danger" @click="deleteRoutine(routine.id)">
            删除
          </van-button>
        </template>
      </van-cell>
    </van-cell-group>

    <!-- 创建/编辑弹出层 -->
    <van-popup v-model:show="showCreate" position="bottom" :style="{ height: '90%' }">
      <div class="routine-form">
        <h3>{{ editingId ? '编辑' : '新建' }}自动化</h3>

        <van-form @submit="onSubmit">
          <van-cell-group inset>
            <van-field
              v-model="formData.name"
              label="名称"
              placeholder="例如：温度过高自动降温"
              :rules="[{ required: true, message: '请输入名称' }]"
            />

            <van-field
              label="设备"
              placeholder="选择设备"
              :model-value="selectedDeviceName"
              readonly
              is-link
              @click="showDevicePicker = true"
            />

            <van-field label="触发逻辑">
              <template #input>
                <van-radio-group v-model="formData.logic" direction="horizontal">
                  <van-radio name="AND">全部满足</van-radio>
                  <van-radio name="OR">任一满足</van-radio>
                </van-radio-group>
              </template>
            </van-field>
          </van-cell-group>

          <!-- 触发器列表 -->
          <van-cell-group inset title="触发条件">
            <van-cell
              v-for="(trigger, index) in formData.triggers"
              :key="index"
              :title="triggerText(trigger)"
            >
              <template #right-icon>
                <van-button size="small" @click="removeTrigger(index)">删除</van-button>
              </template>
            </van-cell>
            <van-cell title="添加条件" is-link @click="showAddTrigger = true" />
          </van-cell-group>

          <!-- 执行动作 -->
          <van-cell-group inset title="执行动作">
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

    <!-- 添加触发器弹出层 -->
    <van-popup v-model:show="showAddTrigger" position="bottom" :style="{ height: '60%' }">
      <div class="trigger-form">
        <h3>添加触发条件</h3>

        <van-form @submit="onAddTrigger">
          <van-cell-group inset>
            <van-field label="类型" readonly @click="showTriggerTypePicker = true">
              <template #input>
                {{ triggerTypeText(newTrigger.type) }}
              </template>
            </van-field>

            <van-field label="条件" readonly @click="showOperatorPicker = true">
              <template #input>
                {{ operatorText(newTrigger.operator) }}
              </template>
            </van-field>

            <van-field
              v-model="newTrigger.value"
              label="值"
              placeholder="请输入"
            />
          </van-cell-group>

          <div class="form-buttons">
            <van-button block @click="showAddTrigger = false">取消</van-button>
            <van-button block type="primary" native-type="submit">添加</van-button>
          </div>
        </van-form>
      </div>
    </van-popup>

    <!-- 选择器们 -->
    <van-popup v-model:show="showDevicePicker" position="bottom">
      <van-picker
        :columns="deviceColumns"
        @confirm="onDeviceConfirm"
        @cancel="showDevicePicker = false"
      />
    </van-popup>

    <van-popup v-model:show="showTriggerTypePicker" position="bottom">
      <van-picker
        :columns="triggerTypeColumns"
        @confirm="onTriggerTypeConfirm"
        @cancel="showTriggerTypePicker = false"
      />
    </van-popup>

    <van-popup v-model:show="showOperatorPicker" position="bottom">
      <van-picker
        :columns="operatorColumns"
        @confirm="onOperatorConfirm"
        @cancel="showOperatorPicker = false"
      />
    </van-popup>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { showToast, showConfirmDialog } from 'vant'
import { useDevicesStore } from '@/stores/devices'
import { routinesApi } from '@/api/automation'
import type { Routine, Trigger } from '@/types/automation'
import dayjs from 'dayjs'

const devicesStore = useDevicesStore()

const routines = ref<Routine[]>([])
const showCreate = ref(false)
const showAddTrigger = ref(false)
const showDevicePicker = ref(false)
const showTriggerTypePicker = ref(false)
const showOperatorPicker = ref(false)
const editingId = ref<number | null>(null)

const formData = ref({
  name: '',
  deviceId: 0,
  logic: 'AND' as 'AND' | 'OR',
  triggers: [] as Trigger[],
  action: {
    power: false,
    mode: 'cool',
    temp: 26,
  },
})

const newTrigger = ref<Trigger>({
  type: 'temp',
  operator: '>',
  value: 28,
})

const devices = computed(() => devicesStore.devices)
const deviceColumns = computed(() =>
  devices.value.map((d) => ({ text: d.name, value: d.id }))
)
const selectedDeviceName = computed(() => {
  const device = devices.value.find((d) => d.id === formData.value.deviceId)
  return device?.name || '请选择设备'
})

const triggerTypeColumns = [
  { text: '温度', value: 'temp' },
  { text: '湿度', value: 'hum' },
  { text: '时间', value: 'time' },
  { text: '星期', value: 'weekday' },
]

const operatorColumns = [
  { text: '大于', value: '>' },
  { text: '小于', value: '<' },
  { text: '等于', value: '=' },
  { text: '大于等于', value: '>=' },
  { text: '小于等于', value: '<=' },
]

const formatDate = (date: string) => {
  return dayjs(date).format('YYYY-MM-DD HH:mm')
}

const actionText = (action: any) => {
  if (!action.power) return '关机'
  return `开机 → ${action.mode === 'cool' ? '制冷' : '制热'} ${action.temp}°C`
}

const triggerTypeText = (type: string) => {
  const map = {
    temp: '温度',
    hum: '湿度',
    time: '时间',
    weekday: '星期',
    date: '日期',
    power: '电源状态',
    mode: '模式',
    current: '电流',
  }
  return map[type] || type
}

const operatorText = (op: string) => {
  const map = { '>': '大于', '<': '小于', '=': '等于', '>=': '大于等于', '<=': '小于等于' }
  return map[op] || op
}

const triggerText = (trigger: Trigger) => {
  return `${triggerTypeText(trigger.type)} ${operatorText(trigger.operator)} ${trigger.value}`
}

const fetchRoutines = async () => {
  try {
    routines.value = await routinesApi.getAll()
  } catch (error) {
    showToast('加载失败')
  }
}

const toggleRoutine = async (routine: Routine) => {
  try {
    await routinesApi.update(routine.id, { enabled: routine.enabled })
    showToast(routine.enabled ? '已启用' : '已禁用')
  } catch (error) {
    showToast('操作失败')
    routine.enabled = !routine.enabled
  }
}

const editRoutine = (routine: Routine) => {
  editingId.value = routine.id
  formData.value = {
    name: routine.name,
    deviceId: routine.deviceId,
    logic: routine.logic,
    triggers: [...routine.triggers],
    action: { ...routine.action },
  }
  showCreate.value = true
}

const deleteRoutine = async (id: number) => {
  try {
    await showConfirmDialog({ message: '确认删除此自动化？' })
    await routinesApi.delete(id)
    await fetchRoutines()
    showToast('删除成功')
  } catch (error) {
    // 用户取消或删除失败
  }
}

const removeTrigger = (index: number) => {
  formData.value.triggers.splice(index, 1)
}

const onDeviceConfirm = ({ selectedValues }) => {
  formData.value.deviceId = selectedValues[0]
  showDevicePicker.value = false
}

const onTriggerTypeConfirm = ({ selectedValues }) => {
  newTrigger.value.type = selectedValues[0]
  showTriggerTypePicker.value = false
}

const onOperatorConfirm = ({ selectedValues }) => {
  newTrigger.value.operator = selectedValues[0]
  showOperatorPicker.value = false
}

const onAddTrigger = () => {
  formData.value.triggers.push({ ...newTrigger.value })
  showAddTrigger.value = false
  newTrigger.value = {
    type: 'temp',
    operator: '>',
    value: 28,
  }
}

const onSubmit = async () => {
  if (formData.value.triggers.length === 0) {
    showToast('请至少添加一个触发条件')
    return
  }

  try {
    if (editingId.value) {
      await routinesApi.update(editingId.value, formData.value)
      showToast('更新成功')
    } else {
      await routinesApi.create(formData.value)
      showToast('创建成功')
    }
    showCreate.value = false
    editingId.value = null
    await fetchRoutines()
  } catch (error) {
    showToast('操作失败')
  }
}

onMounted(() => {
  fetchRoutines()
})
</script>

<style scoped>
.routine-page {
  padding-bottom: 20px;
}

.add-button {
  margin: 16px;
}

.routine-item {
  margin-bottom: 16px;
}

.routine-form,
.trigger-form {
  padding: 16px;
  max-height: 100%;
  overflow-y: auto;
}

.routine-form h3,
.trigger-form h3 {
  margin-bottom: 16px;
  text-align: center;
}

.form-buttons {
  display: flex;
  gap: 12px;
  margin-top: 24px;
}
</style>
