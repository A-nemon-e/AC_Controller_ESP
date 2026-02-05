/*
 * 红外控制器模块 - 实现
 */

#include "ir_controller.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>

// 静态成员初始化
IRsend IRController::irsend(PIN_IR_SEND);
IRrecv IRController::irrecv(PIN_IR_RECV, IR_RECV_BUFFER_SIZE, IR_RECV_TIMEOUT,
                            true);
IRac IRController::ac(PIN_IR_SEND); // ✅ 新增：统一AC控制器
decode_results IRController::results;
void (*IRController::receiveCallback)(decode_results *) = nullptr;
unsigned long IRController::lastSendTime = 0; // ✅ 初始化

void IRController::init() {
  DEBUG_PRINTLN("[红外] 初始化红外模块");

  // 初始化发送器
  irsend.begin();

  // 初始化接收器
  irrecv.enableIRIn();

  DEBUG_PRINTLN("[红外] ✅ 红外模块就绪");
}

bool IRController::sendRaw(const char *rawDataStr) {
  DEBUG_PRINTF("[红外] 发送原始数据: %s\n", rawDataStr);

  // 解析字符串
  uint16_t rawData[512]; // 最多512个时序值
  uint16_t length = parseRawString(rawDataStr, rawData, 512);

  if (length == 0) {
    DEBUG_PRINTLN("[红外] ❌ 数据解析失败");
    return false;
  }

  return sendRaw(rawData, length);
}

bool IRController::sendRaw(uint16_t *rawData, uint16_t length) {
  DEBUG_PRINTF("[红外] 发送%d个时序值\n", length);

  // ✅ 记录发送时间，防止收到回声
  lastSendTime = millis();

  // 发送原始数据（38kHz载波）
  irsend.sendRaw(rawData, length, IR_CARRIER_FREQ);

  DEBUG_PRINTLN("[红外] ✅ 发送完成");
  return true;
}

void IRController::handleReceive() {
  // ✅ 过滤自发自收的回声 (1.5秒盲区)
  if (millis() - lastSendTime < SEND_IGNORE_WINDOW) {
    return;
  }

  if (irrecv.decode(&results)) {
    // 过滤重复码
    if (results.value == 0xFFFFFFFF || results.value == 0x0) {
      irrecv.resume();
      return;
    }

    // 红外LED指示
    LEDIndicator::blinkIR();

    DEBUG_PRINTLN("[红外] 📡 收到红外信号");
    DEBUG_PRINTF("[红外] 协议: ");

    // 打印协议类型
    if (results.decode_type == UNKNOWN) {
      DEBUG_PRINTLN("UNKNOWN (自定义协议)");
    } else {
      DEBUG_PRINTLN(typeToString(results.decode_type));
    }

    DEBUG_PRINTF("[红外] 位数: %d\n", results.bits);
    DEBUG_PRINTF("[红外] 值: 0x%llX\n", results.value);

    // 打印原始数据（用于学习）
    String rawStr = resultsToRawString(&results);
    DEBUG_PRINTF("[红外] 原始数据: %s\n", rawStr.c_str());

    // 触发回调（由主程序处理解析和发布）
    if (receiveCallback != nullptr) {
      receiveCallback(&results);
    }

    // ⚠️ 移除了自动发布事件的代码，现在由主程序的 onIRReceived() 统一处理

    // 准备接收下一个信号
    irrecv.resume();
  }
}

String IRController::getLastRawData() { return resultsToRawString(&results); }

void IRController::setReceiveCallback(void (*callback)(decode_results *)) {
  receiveCallback = callback;
}

uint16_t IRController::parseRawString(const char *str, uint16_t *buffer,
                                      uint16_t maxLen) {
  uint16_t count = 0;
  char *strCopy = strdup(str);
  char *token = strtok(strCopy, ",");

  while (token != nullptr && count < maxLen) {
    buffer[count++] = atoi(token);
    token = strtok(nullptr, ",");
  }

  free(strCopy);
  return count;
}

String IRController::resultsToRawString(decode_results *results) {
  String rawStr = "";

  // 将rawbuf转换为逗号分隔的字符串
  for (uint16_t i = 1; i < results->rawlen; i++) {
    if (i > 1)
      rawStr += ",";
    rawStr += String(results->rawbuf[i] * kRawTick);
  }

  return rawStr;
}

// ===== ✅ 新增：品牌协议支持 =====

bool IRController::sendBrand(const char *brand, int model, bool power,
                             const char *mode, uint8_t temp, uint8_t fanSpeed,
                             bool swingV, bool swingH) {
  DEBUG_PRINTF("[红外] 发送品牌协议: %s (型号: %d)\n", brand, model);

  // ✅ 记录发送时间，防止收到回声
  lastSendTime = millis();

  // 1. 转换品牌字符串为协议类型
  decode_type_t protocol = stringToProtocol(brand);
  if (protocol == decode_type_t::UNKNOWN) {
    DEBUG_PRINTLN("[红外] ❌ 不支持的品牌");
    return false;
  }

  // 2. 构建空调状态
  stdAc::state_t state;
  state.protocol = protocol;
  state.model = model;
  state.power = power;

  // 3. 转换模式
  if (strcmp(mode, "cool") == 0) {
    state.mode = stdAc::opmode_t::kCool;
  } else if (strcmp(mode, "heat") == 0) {
    state.mode = stdAc::opmode_t::kHeat;
  } else if (strcmp(mode, "dry") == 0) {
    state.mode = stdAc::opmode_t::kDry;
  } else if (strcmp(mode, "fan") == 0 || strcmp(mode, "fan_only") == 0) {
    state.mode = stdAc::opmode_t::kFan;
  } else {
    state.mode = stdAc::opmode_t::kAuto;
  }

  // 4. 设置温度和风速
  state.degrees = temp;

  switch (fanSpeed) {
  case 1:
    state.fanspeed = stdAc::fanspeed_t::kLow;
    break;
  case 2:
    state.fanspeed = stdAc::fanspeed_t::kMedium;
    break;
  case 3:
    state.fanspeed = stdAc::fanspeed_t::kHigh;
    break;
  default:
    state.fanspeed = stdAc::fanspeed_t::kAuto;
    break;
  }

  // 5. 设置摆风
  state.swingv = swingV ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
  state.swingh = swingH ? stdAc::swingh_t::kAuto : stdAc::swingh_t::kOff;

  // 6. 发送
  DEBUG_PRINTF("[红外] 参数: Power=%d, Mode=%d, Temp=%d, Fan=%d\n", power,
               state.mode, temp, fanSpeed);

  bool success = ac.sendAc(state);

  if (success) {
    DEBUG_PRINTLN("[红外] ✅ 品牌协议发送成功");
  } else {
    DEBUG_PRINTLN("[红外] ❌ 品牌协议发送失败");
  }

  return success;
}

decode_type_t IRController::stringToProtocol(const char *brand) {
  // ✅ 优化：直接使用 IRremoteESP8266 库提供的转换函数
  return strToDecodeType(brand);
}

String IRController::getSupportedBrandsJSON() {
  // ✅ 改为堆分配
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  DEBUG_PRINTLN("[红外] 生成支持品牌列表...");

  // 遍历所有可能的协议ID (kLastDecodeType from IRremoteESP8266.h)
  for (int i = 1; i <= kLastDecodeType; i++) {
    decode_type_t protocol = (decode_type_t)i;

    // 检查是否为空调协议
    if (IRac::isProtocolSupported(protocol)) {
      String name = typeToString(protocol);
      name.toUpperCase(); // 统一转大写
      array.add(name);
    }
  }

  String jsonString;
  serializeJson(doc, jsonString);

  DEBUG_PRINTF("[红外] 支持 %d 个品牌\n", array.size());
  return jsonString;
}
