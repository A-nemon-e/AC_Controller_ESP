<template>
    <van-popup v-model:show="visible" position="bottom" :style="{ height: '90%' }" round closeable
        @closed="handleClosed">
        <div class="wizard-container">
            <!-- 标题栏 -->
            <div class="wizard-header">
                <h2 class="title">配置向导</h2>
                <van-steps :active="activeStep" active-color="#1989fa">
                    <van-step>自动检测</van-step>
                    <van-step>手动选择</van-step>
                    <van-step>红外学习</van-step>
                </van-steps>
            </div>

            <!-- 内容区域 -->
            <div class="wizard-content">
                <!-- 步骤 1: 自动检测 -->
                <div v-if="activeStep === 0" class="step-content">
                    <div class="auto-detect-box">
                        <div class="radar-scan" :class="{ scanning: isScanning }">
                            <van-icon name="aim" size="80" :color="isScanning ? '#1989fa' : '#ccc'" />
                            <div class="ripple" v-if="isScanning"></div>
                        </div>
                        <h3 class="status-text">{{ scanStatusText }}</h3>
                        <p class="tip-text">请将遥控器对准设备，按下“开机”键或“制冷26度”</p>

                        <div class="action-area">
                            <van-button v-if="!isScanning" type="primary" block icon="play-circle-o" @click="startAutoScan">
                                开始自动检测
                            </van-button>
                            <van-button v-else type="danger" block icon="stop-circle-o" plain @click="stopAutoScan">
                                停止检测
                            </van-button>
                        </div>
                    </div>
                </div>

                <!-- 步骤 2: 手动选择 -->
                <div v-else-if="activeStep === 1" class="step-content">
                    <van-search v-model="searchText" placeholder="搜索品牌 (如: Gree)" />

                    <div v-if="loadingBrands" class="loading-box">
                        <van-loading type="spinner" color="#1989fa" vertical>从固件加载品牌库...</van-loading>
                    </div>

                    <div v-else class="brand-list">
                        <van-empty v-if="filteredBrands.length === 0" description="未找到匹配品牌" />
                        
                        <van-collapse v-model="activeBrandNames" accordion>
                            <van-collapse-item v-for="brand in filteredBrands" :key="brand.name" :name="brand.name">
                                <template #title>
                                    <div class="brand-title">
                                        <span class="brand-name">{{ brand.name }}</span>
                                        <van-tag type="primary" plain>{{ brand.models.length }} 个型号</van-tag>
                                    </div>
                                </template>
                                
                                <div class="model-grid">
                                    <div v-for="model in brand.models" :key="model" 
                                        class="model-item" 
                                        :class="{ active: selectedBrand?.name === brand.name && selectedBrand?.model === model }"
                                        @click="testModel(brand.name, model)">
                                        <span class="model-label">型号 {{ model }}</span>
                                        <van-icon name="play-circle-o" class="play-icon" />
                                    </div>
                                </div>
                                <p class="test-tip">点击型号立即发送测试指令 (制冷 26°C)</p>
                            </van-collapse-item>
                        </van-collapse>
                    </div>
                </div>

                <!-- 步骤 3: 红外学习 -->
                <div v-else-if="activeStep === 2" class="step-content">
                    <van-notice-bar left-icon="info-o" text="如果找不到对应品牌，请逐个录制按键。" />
                    
                    <div class="scene-list">
                        <div v-for="(scene, index) in scenes" :key="index" class="scene-item">
                            <div class="scene-info" @click="openRenameDialog(index)">
                                <div class="scene-name-row">
                                    <div class="scene-name">{{ scene.name }}</div>
                                    <van-icon name="edit" class="edit-icon" />
                                </div>
                                <div class="scene-desc">{{ scene.desc }}</div>
                            </div>
                            <div class="scene-actions">
                                <van-button v-if="!scene.raw" size="small" type="primary" plain icon="aim" 
                                    :loading="learningIndex === index"
                                    @click="startLearn(index)">
                                    录制
                                </van-button>
                                <template v-else>
                                    <van-button size="small" icon="play-circle-o" @click="testScene(index)"></van-button>
                                    <van-button size="small" type="danger" plain icon="delete-o" @click="clearScene(index)"></van-button>
                                    <van-tag type="success" mark>已录制</van-tag>
                                </template>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- 底部按钮 -->
            <div class="wizard-footer">
                <van-button v-if="activeStep > 0" plain type="default" @click="prevStep">上一步</van-button>
                <div class="spacer"></div>
                
                <van-button v-if="activeStep < 2" type="primary" @click="nextStep">
                    {{ activeStep === 0 ? '跳过，手动选择' : '下一步' }}
                </van-button>
                <van-button v-else type="success" :loading="saving" @click="finishSetup">
                    完成配置
                </van-button>
            </div>
        </div>
        
        <!-- 重命名弹窗 -->
        <van-dialog v-model:show="showRenameDialog" title="重命名场景" show-cancel-button @confirm="confirmRename">
            <van-field v-model="renameText" placeholder="请输入场景名称" />
        </van-dialog>
    </van-popup>
</template>

<script setup lang="ts">
import { ref, computed, watch, onUnmounted } from 'vue';
import { showToast, showSuccessToast, showFailToast } from 'vant';
import { devicesApi } from '@/api/devices';

const props = defineProps<{
    modelValue: boolean;
    deviceId: number;
    userId: number; // Optional context
}>();

const emit = defineEmits(['update:modelValue', 'completed']);

const visible = computed({
    get: () => props.modelValue,
    set: (val) => emit('update:modelValue', val),
});

const activeStep = ref(0);
const COMPLETED_STEP_KEY = 'ac_setup_step';

// ===== Step 1: Auto Detect =====
const isScanning = ref(false);
const scanStatusText = ref('准备就绪');
let pollTimer: any = null;

const startAutoScan = async () => {
    try {
        isScanning.value = true;
        scanStatusText.value = '正在监听红外信号...';
        await devicesApi.startAutoDetect(props.deviceId);
        
        // 轮询状态 (由 MQTT 推送状态更好，这里简单轮询)
        pollTimer = setInterval(checkAutoDetectStatus, 2000);
    } catch (err) {
        isScanning.value = false;
        showFailToast('启动失败');
    }
};

const stopAutoScan = async () => {
    try {
        await devicesApi.stopAutoDetect(props.deviceId);
    } finally {
        isScanning.value = false;
        scanStatusText.value = '已停止';
        if (pollTimer) clearInterval(pollTimer);
    }
};

const checkAutoDetectStatus = async () => {
    try {
        const res: any = await devicesApi.getAutoDetectStatus(props.deviceId);
        
        if (res.status === 'detecting') {
            scanStatusText.value = res.message || '正在监听红外信号...';
        } else if (res.status === 'success') {
            // 检测成功
            isScanning.value = false;
            if (pollTimer) clearInterval(pollTimer);
            
            const result = res.result || {};
            const protocol = result.protocol || 'Unknown';
            const model = result.model || 0;
            
            showSuccessToast(`识别成功: ${protocol} (Model ${model})`);
            
            showSuccessToast(`识别成功: ${protocol} (Model ${model})`);
            
            // ✅ 自动保存并结束向导
            try {
                // 直接保存品牌和型号
                await devicesApi.setBrand(props.deviceId, protocol, model);
                
                setTimeout(() => {
                    showSuccessToast('配置已保存');
                    emit('completed');
                    visible.value = false;
                }, 1500);
            } catch (e) {
                showFailToast('保存配置失败');
            }
            // 不需要跳转 activeStep.value = 1;
            
        } else if (res.status === 'fail' || res.status === 'timeout') {
            isScanning.value = false;
            scanStatusText.value = '检测超时或失败';
            if (pollTimer) clearInterval(pollTimer);
            showFailToast('未检测到信号');
        }
    } catch (e) {
        console.error('Check status failed', e);
    }
};

// ===== Step 2: Manual Selection =====
// 品牌结构: { name: 'GREE', models: [0,1,2...] }
interface BrandItem {
    name: string;
    models: number[];
}
const brands = ref<BrandItem[]>([]);
const loadingBrands = ref(false);
const searchText = ref('');
const activeBrandNames = ref<string[]>([]);
const selectedBrand = ref<{ name: string; model: number } | null>(null);

const filteredBrands = computed(() => {
    if (!searchText.value) return brands.value;
    const lower = searchText.value.toLowerCase();
    return brands.value.filter(b => b.name.toLowerCase().includes(lower));
});

// 加载品牌列表 (从后端/固件)
const loadBrands = async () => {
    loadingBrands.value = true;
    try {
        // 尝试获取，如果是 loading 则重试
        let retries = 5;
        while (retries > 0) {
            const res = await devicesApi.getSetupBrands(props.deviceId);
            if (res.status === 'ready' && Array.isArray(res.brands)) {
                // 解析 brands
                // 假设固件返回的是 ["GREE", "MIDEA", ...] 或带有model的对象
                // 这里做数据适配
                brands.value = parseBrandList(res.brands);
                loadingBrands.value = false;
                return;
            }
            // 等待 1.5s 重试
            await new Promise(r => setTimeout(r, 1500));
            retries--;
        }
        // 如果超时，不再使用 Mock 数据
        if (!brands.value || brands.value.length === 0) {
            showToast('未从设备获取到品牌列表，请输入品牌名称搜索');
        }
    } catch (error) {
        console.error(error);
        showFailToast('获取品牌列表失败');
    } finally {
        loadingBrands.value = false;
    }
};

const parseBrandList = (list: any[]): BrandItem[] => {
    // 适配逻辑
    if (list.length > 0 && typeof list[0] === 'string') {
        // ✅ 默认 Model 从 1 开始 (IRremote 库惯例)
        return list.map((name: string) => ({ name, models: [1] })); 
    }
    return list;
};

// 移除 Hardcoded Fallback Brands
// const getFallbackBrands = ... (Deleted)

const testModel = async (brand: string, model: number) => {
    try {
        showToast(`发送测试: ${brand} (Model ${model})`);
        selectedBrand.value = { name: brand, model };
        
        // 发送临时指令 (Touch-to-Test)
        await devicesApi.sendCommand(props.deviceId, {
            power: true,
            mode: 'cool',
            temp: 26,
            brand: brand,
            model: model
        });
    } catch (err) {
        showFailToast('指令发送失败');
    }
};

// ===== Step 3: Learning =====
// 7 个固定槽位
const scenes = ref([
    { key: 'off', name: '关机', desc: '关闭空调', raw: '' },
    { key: 'cool_26', name: '舒适制冷', desc: '制冷 26°C / 自动风', raw: '' },
    { key: 'heat_20', name: '舒适制热', desc: '制热 20°C / 自动风', raw: '' },
    { key: 'cool_max', name: '强力制冷', desc: '制冷 18°C / 强风', raw: '' },
    { key: 'heat_max', name: '强力制热', desc: '制热 30°C / 强风', raw: '' },
    { key: 'fan_only', name: '仅送风', desc: '送风模式', raw: '' },
    { key: 'custom_1', name: '自定义1', desc: '用户自定义场景', raw: '' },
]);
const learningIndex = ref<number>(-1);

// 重命名相关状态
const showRenameDialog = ref(false);
const renameText = ref('');
const renameTargetIndex = ref(-1);

const openRenameDialog = (index: number) => {
    renameTargetIndex.value = index;
    renameText.value = scenes.value[index].name;
    showRenameDialog.value = true;
};

const confirmRename = () => {
    if (renameTargetIndex.value >= 0 && renameText.value.trim()) {
        scenes.value[renameTargetIndex.value].name = renameText.value.trim();
        scenes.value[renameTargetIndex.value].desc = '自定义名称'; // 更新描述以反映修改
    }
};

// 轮询学习状态
const checkLearnStatus = async (key: string) => {
    try {
        const res: any = await devicesApi.getLearningResult(props.deviceId);
        
        if (res.status === 'success' && res.key === key) {
            // 学习成功
            if (pollTimer) clearInterval(pollTimer);
            
            // 更新场景数据
            scenes.value[learningIndex.value].raw = res.raw;
            scenes.value[learningIndex.value].hasCode = true;
            scenes.value[learningIndex.value].status = 'success';
            
            showSuccessToast('录制成功');
            learningIndex.value = -1; // 退出学习状态
            
        } else if (res.status === 'timeout' && res.key === key) {
            // 学习超时
            if (pollTimer) clearInterval(pollTimer);
            scenes.value[learningIndex.value].status = 'idle';
            showFailToast('录制超时');
            learningIndex.value = -1;
        }
        // else waiting...
        
    } catch (e) {
        console.error('Check learn status failed', e);
    }
};

const startLearn = async (index: number) => {
    if (learningIndex.value !== -1) return; // 已经在学习中
    
    learningIndex.value = index;
    const scene = scenes.value[index];
    scene.status = 'learning';
    
    try {
        // 调用 API 启动学习
        await devicesApi.startLearning(props.deviceId, scene.key);
        
        showToast({ message: `请按遥控器: ${scene.name}`, duration: 0 }); // 持续显示
        
        // 启动轮询
        pollTimer = setInterval(() => {
            checkLearnStatus(scene.key);
        }, 1000);
        
        // 前端兜底超时 (40s)
        setTimeout(() => {
            if (learningIndex.value === index) {
                if (pollTimer) clearInterval(pollTimer);
                if (scene.status === 'learning') {
                    scene.status = 'idle';
                    showFailToast('前端等待超时');
                    learningIndex.value = -1;
                }
            }
        }, 40000);
        
    } catch (e) {
        console.error('Start learning failed', e);
        scene.status = 'idle';
        learningIndex.value = -1;
        showFailToast('启动失败');
    }
};

const testScene = async (index: number) => {
    const scene = scenes.value[index];
    if (!scene.raw) return;
    await devicesApi.sendCommand(props.deviceId, {
        power: true, // 并不重要，主要是 raw
        raw: scene.raw
    } as any);
    showToast('发送测试数据');
};

const clearScene = (index: number) => {
    scenes.value[index].raw = '';
};

// ===== Navigation =====
const saving = ref(false);

const nextStep = () => {
    if (activeStep.value < 2) {
        activeStep.value++;
    }
};

const prevStep = () => {
    if (activeStep.value > 0) {
        activeStep.value--;
    }
};

const finishSetup = async () => {
    saving.value = true;
    try {
        // 1. 如果选择了品牌，保存品牌配置
        if (selectedBrand.value) {
            await devicesApi.setBrand(props.deviceId, selectedBrand.value.name, selectedBrand.value.model);
        }

        // 2. 如果录制了场景，保存场景
        const learnedScenes = scenes.value
            .filter(s => s.raw)
            .map(s => ({
                key: s.key,
                raw: s.raw,
                // ✅ 传递 name 以便后端保存
                name: s.name,
                // 为了兼容后端 DTO，可能需要填充一些假数据，或者后端已经允许 raw
                power: s.key === 'off' ? false : true
            }));
        
        if (learnedScenes.length > 0) {
            await devicesApi.saveScenes(props.deviceId, learnedScenes);
        }

        showSuccessToast('配置已保存');
        emit('completed');
        visible.value = false;
    } catch (e) {
        showFailToast('保存失败');
    } finally {
        saving.value = false;
    }
};

// 监听 Step 变化
watch(activeStep, (val) => {
    if (val === 1 && brands.value.length === 0) {
        loadBrands();
    }
    if (val !== 0) {
        stopAutoScan(); // 离开第一页自动停止
    }
});

const handleClosed = () => {
    stopAutoScan();
    activeStep.value = 0;
};

// Auto Start Brand Load on Open if step is 1
watch(visible, (val) => {
    if (val && activeStep.value === 1) {
        loadBrands();
    }
});

onUnmounted(() => {
    stopAutoScan();
});
</script>

<style scoped>
.wizard-container {
    height: 100%;
    display: flex;
    flex-direction: column;
    background-color: #f7f8fa;
}

.wizard-header {
    background: #fff;
    padding-bottom: 10px;
    border-bottom: 1px solid #eee;
}

.title {
    text-align: center;
    margin: 16px 0 0;
    font-size: 18px;
    font-weight: 600;
}

.wizard-content {
    flex: 1;
    overflow-y: auto;
    position: relative;
    padding: 0;
}

.step-content {
    padding: 16px;
    height: 100%;
    box-sizing: border-box;
}

/* Step 1: Auto Detect */
.auto-detect-box {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100%;
    text-align: center;
    padding-top: 40px;
}

.radar-scan {
    width: 120px;
    height: 120px;
    border-radius: 50%;
    background: #f0f8ff;
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
    margin-bottom: 24px;
}

.scanning .ripple {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 100%;
    height: 100%;
    border-radius: 50%;
    border: 2px solid #1989fa;
    animation: ripple 2s infinite;
    opacity: 0;
}

@keyframes ripple {
    0% { width: 100%; height: 100%; opacity: 0.8; }
    100% { width: 200%; height: 200%; opacity: 0; }
}

.status-text {
    font-size: 18px;
    color: #333;
    margin-bottom: 8px;
}

.tip-text {
    font-size: 14px;
    color: #999;
    margin-bottom: 40px;
}

.action-area {
    width: 80%;
}

/* Step 2: Manual */
.loading-box {
    display: flex;
    justify-content: center;
    padding: 40px;
}

.brand-list {
    margin-top: 10px;
}

.brand-title {
    display: flex;
    justify-content: space-between;
    align-items: center;
    width: 100%;
}

.brand-name {
    font-weight: 600;
}

.model-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
    padding: 10px 0;
}

.model-item {
    background: #f2f2f2;
    padding: 8px;
    border-radius: 4px;
    text-align: center;
    font-size: 12px;
    cursor: pointer;
    position: relative;
    border: 1px solid transparent;
}

.model-item.active {
    background: #e6f1fe;
    color: #1989fa;
    border-color: #1989fa;
}

.model-item .play-icon {
    font-size: 16px;
    display: block;
    margin: 4px auto 0;
    color: #1989fa;
}

.test-tip {
    font-size: 10px;
    color: #999;
    text-align: center;
    margin-top: 8px;
}

/* Step 3: Learn */
.scene-list {
    margin-top: 10px;
}

.scene-item {
    background: #fff;
    padding: 12px;
    border-radius: 8px;
    margin-bottom: 10px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 1px 4px rgba(0,0,0,0.05);
}

.scene-info {
    flex: 1;
    /* ✅ 可点击以编辑 */
    cursor: pointer;
    display: flex;
    flex-direction: column;
}

.scene-name-row {
    display: flex;
    align-items: center;
    gap: 4px;
}

.scene-name {
    font-size: 16px;
    font-weight: 500;
}

.edit-icon {
    font-size: 14px;
    color: #999;
}

.scene-desc {
    font-size: 12px;
    color: #999;
}

.scene-actions {
    display: flex;
    gap: 8px;
    align-items: center;
}

/* Footer */
.wizard-footer {
    background: #fff;
    border-top: 1px solid #eee;
    padding: 12px 16px;
    display: flex;
    align-items: center;
}

.spacer {
    flex: 1;
}
</style>
