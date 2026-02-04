/*
 * 学习场景管理器 - 实现
 */

#include "scene_manager.h"

// 静态成员初始化
LearnedScenes SceneManager::scenes = {0};

void SceneManager::init() {
  DEBUG_PRINTLN("[场景] 初始化场景管理器");

  // 尝试从EEPROM加载
  if (!load()) {
    DEBUG_PRINTLN("[场景] 未找到有效场景，使用空列表");
    scenes.count = 0;
    scenes.checksum = 0;
    save(); // 保存空列表
  } else {
    DEBUG_PRINTF("[场景] ✅ 加载了 %d 个场景\n", scenes.count);
  }
}

bool SceneManager::addScene(const char *key, const char *rawData, bool power,
                            const char *mode, uint8_t temp) {
  // 检查是否已存在同key场景
  int existIndex = -1;
  for (uint8_t i = 0; i < scenes.count; i++) {
    if (strcmp(scenes.scenes[i].key, key) == 0) {
      existIndex = i;
      break;
    }
  }

  LearnedScene *targetScene;

  if (existIndex >= 0) {
    // 更新现有场景
    targetScene = &scenes.scenes[existIndex];
    DEBUG_PRINTF("[场景] 更新现有场景: %s\n", key);
  } else {
    // 添加新场景
    if (scenes.count >= 8) {
      DEBUG_PRINTLN("[场景] ❌ 场景数量已达上限（8个）");
      return false;
    }
    targetScene = &scenes.scenes[scenes.count];
    scenes.count++;
    DEBUG_PRINTF("[场景] 添加新场景: %s (总数: %d)\n", key, scenes.count);
  }

  // 填充场景数据
  strncpy(targetScene->key, key, sizeof(targetScene->key) - 1);
  targetScene->key[sizeof(targetScene->key) - 1] = '\0';

  strncpy(targetScene->rawData, rawData, sizeof(targetScene->rawData) - 1);
  targetScene->rawData[sizeof(targetScene->rawData) - 1] = '\0';

  targetScene->power = power;

  strncpy(targetScene->mode, mode, sizeof(targetScene->mode) - 1);
  targetScene->mode[sizeof(targetScene->mode) - 1] = '\0';

  targetScene->temp = temp;

  // 计算校验和
  targetScene->checksum = calculateChecksum(*targetScene);

  // 保存到EEPROM
  if (save()) {
    DEBUG_PRINTLN("[场景] ✅ 场景已保存到EEPROM");
    return true;
  } else {
    DEBUG_PRINTLN("[ 场景] ❌ 场景保存失败");
    return false;
  }
}

bool SceneManager::matchScene(const char *rawData, LearnedScene &outScene) {
  for (uint8_t i = 0; i < scenes.count; i++) {
    if (compareRawData(rawData, scenes.scenes[i].rawData)) {
      outScene = scenes.scenes[i];
      DEBUG_PRINTF("[场景] ✅ 匹配到场景: %s\n", outScene.key);
      return true;
    }
  }

  DEBUG_PRINTLN("[场景] ❌ 未匹配到任何场景");
  return false;
}

bool SceneManager::getScene(const char *key, LearnedScene &outScene) {
  for (uint8_t i = 0; i < scenes.count; i++) {
    if (strcmp(scenes.scenes[i].key, key) == 0) {
      outScene = scenes.scenes[i];
      return true;
    }
  }
  return false;
}

void SceneManager::clearScenes() {
  DEBUG_PRINTLN("[场景] 清空所有场景");
  scenes.count = 0;
  scenes.checksum = 0;
  save();
}

uint8_t SceneManager::getSceneCount() { return scenes.count; }

void SceneManager::printScenes() {
  DEBUG_PRINTLN("\n========== 学习场景 ==========");
  DEBUG_PRINTF("场景数量: %d/8\n", scenes.count);

  for (uint8_t i = 0; i < scenes.count; i++) {
    LearnedScene &s = scenes.scenes[i];
    DEBUG_PRINTF("\n[%d] %s\n", i + 1, s.key);
    DEBUG_PRINTF("  电源: %s\n", s.power ? "开" : "关");
    DEBUG_PRINTF("  模式: %s\n", s.mode);
    DEBUG_PRINTF("  温度: %d°C\n", s.temp);
    DEBUG_PRINTF("  Raw长度: %d\n", strlen(s.rawData));
  }

  DEBUG_PRINTLN("==============================\n");
}

bool SceneManager::save() {
  // ✅ 强制刷新
  EEPROM.end();
  EEPROM.begin(EEPROM_SIZE);

  // 计算全局校验和
  scenes.checksum = calculateGlobalChecksum();

  // 写入EEPROM
  EEPROM.put(EEPROM_SCENES_ADDR, scenes);
  EEPROM.commit();

  DEBUG_PRINTLN("[场景] EEPROM保存完成");
  return true;
}

bool SceneManager::load() {
  // ✅ 强制刷新
  EEPROM.end();
  EEPROM.begin(EEPROM_SIZE);

  // 从EEPROM读取
  EEPROM.get(EEPROM_SCENES_ADDR, scenes);

  // 验证全局校验和
  if (!verifyGlobalChecksum()) {
    DEBUG_PRINTLN("[场景] ❌ 全局校验和错误");
    return false;
  }

  // 验证每个场景的校验和
  for (uint8_t i = 0; i < scenes.count; i++) {
    if (!verifyChecksum(scenes.scenes[i])) {
      DEBUG_PRINTF("[场景] ❌ 场景 %d 校验和错误\n", i);
      return false;
    }
  }

  return true;
}

uint16_t SceneManager::calculateChecksum(const LearnedScene &scene) {
  uint16_t sum = 0;
  const uint8_t *ptr = (const uint8_t *)&scene;

  // 计算除checksum外的所有字节
  for (size_t i = 0; i < sizeof(LearnedScene) - sizeof(uint16_t); i++) {
    sum ^= ptr[i];
    sum = (sum << 1) | (sum >> 15); // 循环左移
  }

  return sum;
}

uint16_t SceneManager::calculateGlobalChecksum() {
  uint16_t sum = 0;
  const uint8_t *ptr = (const uint8_t *)&scenes;

  // 计算除checksum外的所有字节
  for (size_t i = 0; i < sizeof(LearnedScenes) - sizeof(uint16_t); i++) {
    sum ^= ptr[i];
    sum = (sum << 1) | (sum >> 15);
  }

  return sum;
}

bool SceneManager::verifyChecksum(const LearnedScene &scene) {
  uint16_t calculated = calculateChecksum(scene);
  return calculated == scene.checksum;
}

bool SceneManager::verifyGlobalChecksum() {
  uint16_t calculated = calculateGlobalChecksum();
  return calculated == scenes.checksum;
}

bool SceneManager::compareRawData(const char *data1, const char *data2) {
  // 解析两个时序数据字符串并比较
  // 格式示例："2928,1830,364,1188,366,1186,368,460,..."

  // 简化实现：直接字符串比较（后续可优化为容错匹配）
  // TODO: 实现±5%误差容忍的智能匹配

  int len1 = strlen(data1);
  int len2 = strlen(data2);

  // 长度差异超过10%认为不匹配
  if (abs(len1 - len2) > len1 / 10) {
    return false;
  }

  // 简单字符串匹配（可以改进）
  return strcmp(data1, data2) == 0;
}
