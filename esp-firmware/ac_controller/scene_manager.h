/*
 * 学习场景管理器 - 头文件
 *
 * 功能：
 * - 存储学习的红外场景到EEPROM
 * - 匹配接收到的红外信号
 * - 管理场景列表
 */

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "config.h"
#include <Arduino.h>
#include <EEPROM.h>

// 单个学习场景
struct LearnedScene {
  char key[16];      // 场景标识（如 "cool_26"）
  char rawData[448]; // 原始时序数据（压缩存储）
  bool power;        // 电源状态
  char mode[8];      // 模式（cool/heat/fan/dry/auto）
  uint8_t temp;      // 温度
  uint16_t checksum; // 校验和
};

// 学习场景集合（最多7个场景，适配4KB EEPROM）
struct LearnedScenes {
  uint8_t count;          // 已学习场景数量
  LearnedScene scenes[7]; // ✅ 减少到7个，防止溢出 (每个约476字节)
  uint16_t checksum;      // 整体校验和
};

class SceneManager {
public:
  // 初始化（从EEPROM加载）
  static void init();

  // 添加新场景
  static bool addScene(const char *key, const char *rawData, bool power,
                       const char *mode, uint8_t temp);

  // 匹配原始红外数据（返回是否匹配成功）
  static bool matchScene(const char *rawData, LearnedScene &outScene);

  // 根据key获取场景
  static bool getScene(const char *key, LearnedScene &outScene);

  // 清空所有场景
  static void clearScenes();

  // 获取场景数量
  static uint8_t getSceneCount();

  // 打印所有场景
  static void printScenes();

private:
  static LearnedScenes scenes;

  // 保存到EEPROM
  static bool save();

  // 从EEPROM加载
  static bool load();

  // 计算校验和
  static uint16_t calculateChecksum(const LearnedScene &scene);
  static uint16_t calculateGlobalChecksum();

  // 验证校验和
  static bool verifyChecksum(const LearnedScene &scene);
  static bool verifyGlobalChecksum();

  // 比较原始数据（允许±5%误差）
  static bool compareRawData(const char *data1, const char *data2);
};

#endif // SCENE_MANAGER_H
