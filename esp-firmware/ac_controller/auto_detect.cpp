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
  result.power = false; // Default
  result.temp = 26;     // Default

  // 1. Check overflow
  if (results->overflow) {
    DEBUG_PRINTLN("[自动检测] ⚠️ 缓冲区溢出！");
    result.protocol = "OVERFLOW";
    return result;
  }

  // 2. Use IRac to decode everything (Protocol + Model + State)
  stdAc::state_t state;
  // Use IRAcUtils to decode
  if (IRAcUtils::decodeToState(results, &state)) {
    DEBUG_PRINTLN("[自动检测] ✅ IRac 成功识别信号！");

    result.success = true;
    result.isAC = true;
    result.protocol = typeToString(state.protocol);
    result.model = state.model; // Key: Internal Model ID

    result.power = state.power;
    result.temp = state.degrees;

    // Map Mode
    switch (state.mode) {
    case stdAc::opmode_t::kCool:
      result.mode = "cool";
      break;
    case stdAc::opmode_t::kHeat:
      result.mode = "heat";
      break;
    case stdAc::opmode_t::kDry:
      result.mode = "dry";
      break;
    case stdAc::opmode_t::kFan:
      result.mode = "fan";
      break;
    case stdAc::opmode_t::kAuto:
      result.mode = "auto";
      break;
    default:
      result.mode = "auto";
      break;
    }

    // Map Fan
    switch (state.fanspeed) {
    case stdAc::fanspeed_t::kLow:
      result.fan = 1;
      break;
    case stdAc::fanspeed_t::kMedium:
      result.fan = 2;
      break;
    case stdAc::fanspeed_t::kHigh:
      result.fan = 3;
      break;
    default:
      result.fan = 0;
      break;
    }

    // Map Swing
    result.swingV = (state.swingv != stdAc::swingv_t::kOff);
    result.swingH = (state.swingh != stdAc::swingh_t::kOff);

    // Human Readable Description
    result.description =
        result.protocol + " (Model " + String(result.model) + ")";

    DEBUG_PRINTF("[自动检测] 结果: %s, Power=%d, Temp=%d\n",
                 result.description.c_str(), result.power, result.temp);

  } else {
    // Fallback: Not a standard AC signal
    DEBUG_PRINTLN("[自动检测] ❌ 未识别为空调信号");

    if (results->decode_type != decode_type_t::UNKNOWN) {
      // Known protocol but unknown content
      result.success = true; // Still consider it a success for Protocol ID
      result.protocol = typeToString(results->decode_type);
      result.description = "Known Protocol (Not AC)";
    } else {
      result.protocol = "UNKNOWN";
      result.rawData = resultToSourceCode(results);
    }
  }

  return result;
}

// 移除冗余的 extractModel 和 parseACState 实现
int AutoDetect::extractModel(decode_type_t protocol, uint8_t *state) {
  return 0;
}
void AutoDetect::parseACState(decode_results *results,
                              DetectionResult &result) {}

String AutoDetect::getProtocolName(decode_type_t type) {
  // 返回协议名称字符串
  return typeToString(type);
}

bool AutoDetect::isACProtocol(decode_type_t type) {
  // 判断是否是空调协议
  return hasACState(type);
}
