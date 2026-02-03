/*
 * 自动协议检测模块 - 实现
 */

#include "auto_detect.h"

// 静态成员初始化
bool AutoDetect::detecting = false;
unsigned long AutoDetect::startTime = 0;

void AutoDetect::start() {
  detecting = true;
  startTime = millis();
  DEBUG_PRINTLN("[自动检测] ✅ 已启动，请在30秒内按下遥控器任意键");
  DEBUG_PRINTLN("[自动检测] 💡 建议按：开机键 或 制冷26度");
}

void AutoDetect::stop() {
  detecting = false;
  DEBUG_PRINTLN("[自动检测] ⏹ 已停止");
}

bool AutoDetect::isDetecting() {
  // 检查超时
  if (detecting && (millis() - startTime > DETECT_TIMEOUT)) {
    DEBUG_PRINTLN("[自动检测] ⏱ 超时，自动停止");
    stop();
    return false;
  }
  return detecting;
}

String AutoDetect::getStatus() {
  if (!detecting) {
    return "idle";
  }

  uint32_t elapsed = millis() - startTime;
  uint32_t remaining = (DETECT_TIMEOUT - elapsed) / 1000;

  return String("detecting:") + remaining + "s";
}

DetectionResult AutoDetect::analyze(decode_results *results) {
  DetectionResult result;
  result.success = false;
  result.isAC = false;
  result.model = 0;
  result.power = false;
  result.temp = 26;
  result.fan = 0;
  result.swingV = false;
  result.swingH = false;

  // 1. 检查缓冲区溢出
  if (results->overflow) {
    DEBUG_PRINTLN("[自动检测] ⚠️ 缓冲区溢出！信号太长");
    DEBUG_PRINTLN("[自动检测] 建议增加 kCaptureBufferSize");
    result.protocol = "OVERFLOW";
    return result;
  }

  // 2. 获取协议类型
  decode_type_t type = results->decode_type;
  result.protocol = getProtocolName(type);

  DEBUG_PRINTF("[自动检测] 📡 接收信号: Protocol=%s, Bits=%d\n",
               result.protocol.c_str(), results->bits);

  // 3. 检查是否是未知协议
  if (type == decode_type_t::UNKNOWN) {
    DEBUG_PRINTLN("[自动检测] ❌ 协议未识别 (UNKNOWN)");
    DEBUG_PRINTLN("[自动检测] → 建议使用手动选择或Raw学习模式");

    // 保存原始数据用于Raw模式
    result.rawData = resultToSourceCode(results);
    DEBUG_PRINTF("[自动检测] Raw数据长度: %d\n", result.rawData.length());

    return result;
  }

  // 4. ✅ 识别成功！
  result.success = true;
  result.isAC = isACProtocol(type);

  DEBUG_PRINTF("[自动检测] ✅ 协议识别成功: %s\n", result.protocol.c_str());
  DEBUG_PRINTF("[自动检测] 是否空调协议: %s\n", result.isAC ? "是" : "否");

  // 5. 提取型号
  if (results->state != nullptr && result.isAC) {
    result.model = extractModel(type, results->state);
    DEBUG_PRINTF("[自动检测] 型号代码: %d\n", result.model);
  }

  // 6. 解析AC状态（如果是空调协议）
  if (result.isAC) {
    parseACState(results, result);

    // 生成人类可读描述
    result.description = IRAcUtils::resultAcToString(results);
    if (result.description.length() > 0) {
      DEBUG_PRINTLN("[自动检测] 📋 空调状态:");
      DEBUG_PRINTLN(result.description);
    }
  }

  return result;
}

int AutoDetect::extractModel(decode_type_t protocol, uint8_t *state) {
  if (state == nullptr) {
    return 0;
  }

  switch (protocol) {
  case decode_type_t::GREE: {
    // GREE型号在state[4]的bit 4-7
    // YAC=0, YAA=1, YAP=2, YB0=3
    IRGreeAC ac(0);
    ac.setRaw(state);
    // 注意：IRGreeAC可能没有getModel()，需要手动解析
    // 这是一个示例，实际可能需要调整
    uint8_t modelBits = (state[4] >> 4) & 0x0F;
    DEBUG_PRINTF("[自动检测] GREE型号位: 0x%02X\n", modelBits);
    return modelBits;
  }

  case decode_type_t::DAIKIN: {
    // DAIKIN - 注意：IRDaikinESP可能没有getModel()方法
    // 根据实际IRremoteESP8266版本决定是否支持
    // IRDaikinESP ac(0);
    // ac.setRaw(state);
    // return ac.getModel();
    return 0; // 暂时返回默认型号
  }

  case decode_type_t::FUJITSU_AC: {
    IRFujitsuAC ac(0);
    // setRaw需要长度参数，根据Fujitsu协议不同长度也不同
    // ac.setRaw(state, kFujitsuAcStateLength);  // 需要知道具体长度
    // return ac.getModel();
    return 0; // 暂时返回默认型号
  }

  case decode_type_t::MITSUBISHI_AC: {
    // Mitsubishi可能有子型号
    // 需要查阅具体协议文档
    return 0;
  }

  case decode_type_t::MIDEA:
  case decode_type_t::HAIER_AC:
  case decode_type_t::COOLIX:
  default:
    // 这些品牌通常没有明确的型号区分，使用默认0
    return 0;
  }
}

void AutoDetect::parseACState(decode_results *results,
                              DetectionResult &result) {
  decode_type_t type = results->decode_type;
  uint8_t *state = results->state;

  if (state == nullptr) {
    return;
  }

  // 根据不同品牌解析状态
  // 这里展示几个主要品牌的解析方式

  switch (type) {
  case decode_type_t::GREE: {
    IRGreeAC ac(0);
    ac.setRaw(state);

    result.power = ac.getPower();
    result.temp = ac.getTemp();
    result.fan = ac.getFan();
    result.swingV = ac.getSwingVerticalAuto();

    // 模式转换
    switch (ac.getMode()) {
    case kGreeAuto:
      result.mode = "auto";
      break;
    case kGreeCool:
      result.mode = "cool";
      break;
    case kGreeHeat:
      result.mode = "heat";
      break;
    case kGreeDry:
      result.mode = "dry";
      break;
    case kGreeFan:
      result.mode = "fan";
      break;
    default:
      result.mode = "auto";
      break;
    }

    DEBUG_PRINTF("[GREE] Power=%d, Mode=%s, Temp=%d, Fan=%d\n", result.power,
                 result.mode.c_str(), result.temp, result.fan);
    break;
  }

  case decode_type_t::MIDEA: {
    IRMideaAC ac(0);
    // MIDEA使用64bit state，需要转换
    uint64_t stateValue = 0;
    if (results->bits == 48) {
      // MIDEA协议是48bit
      for (int i = 0; i < 6; i++) {
        stateValue |= ((uint64_t)state[i]) << (i * 8);
      }
      ac.setRaw(stateValue);
    } else {
      // 如果是其他长度，跳过设置
      break;
    }

    result.power = ac.getPower();
    result.temp = ac.getTemp();
    result.fan = ac.getFan();
    result.swingV = ac.getSwingVToggle();

    // 模式转换
    switch (ac.getMode()) {
    case kMideaACAuto:
      result.mode = "auto";
      break;
    case kMideaACCool:
      result.mode = "cool";
      break;
    case kMideaACHeat:
      result.mode = "heat";
      break;
    case kMideaACDry:
      result.mode = "dry";
      break;
    case kMideaACFan:
      result.mode = "fan";
      break;
    default:
      result.mode = "auto";
      break;
    }

    DEBUG_PRINTF("[MIDEA] Power=%d, Mode=%s, Temp=%d, Fan=%d\n", result.power,
                 result.mode.c_str(), result.temp, result.fan);
    break;
  }

  case decode_type_t::DAIKIN: {
    IRDaikinESP ac(0);
    ac.setRaw(state);

    result.power = ac.getPower();
    result.temp = ac.getTemp();
    result.fan = ac.getFan();
    result.swingV = ac.getSwingVertical();
    result.swingH = ac.getSwingHorizontal();

    // 模式转换
    switch (ac.getMode()) {
    case kDaikinAuto:
      result.mode = "auto";
      break;
    case kDaikinCool:
      result.mode = "cool";
      break;
    case kDaikinHeat:
      result.mode = "heat";
      break;
    case kDaikinDry:
      result.mode = "dry";
      break;
    case kDaikinFan:
      result.mode = "fan";
      break;
    default:
      result.mode = "auto";
      break;
    }

    DEBUG_PRINTF("[DAIKIN] Power=%d, Mode=%s, Temp=%d, Fan=%d\n", result.power,
                 result.mode.c_str(), result.temp, result.fan);
    break;
  }

    // 可以继续添加其他品牌的解析
    // case decode_type_t::HAIER_AC:
    // case decode_type_t::FUJITSU_AC:
    // 等等...

  default:
    // 对于其他AC协议，使用通用的resultAcToString已经足够
    DEBUG_PRINTLN("[自动检测] 使用通用AC解析");
    break;
  }
}

String AutoDetect::getProtocolName(decode_type_t type) {
  // 返回协议名称字符串
  return typeToString(type);
}

bool AutoDetect::isACProtocol(decode_type_t type) {
  // 判断是否是空调协议
  return hasACState(type);
}
