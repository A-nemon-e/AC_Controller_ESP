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

  // 发送原始数据（38kHz载波）
  irsend.sendRaw(rawData, length, IR_CARRIER_FREQ);

  DEBUG_PRINTLN("[红外] ✅ 发送完成");
  return true;
}

void IRController::handleReceive() {
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

    // 触发回调
    if (receiveCallback != nullptr) {
      receiveCallback(&results);
    }

    // 发布到MQTT（状态更新）
    if (MQTTClient::isConnected()) {
      StaticJsonDocument<512> doc;
      doc["source"] = "ir_recv";
      doc["protocol"] = typeToString(results.decode_type);
      doc["value"] = String((unsigned long long)results.value, HEX);
      doc["bits"] = results.bits;
      doc["raw"] = rawStr;

      char payload[512];
      serializeJson(doc, payload);

      String topic = MQTTClient::getTopic("event");
      MQTTClient::publish(topic.c_str(), payload);
    }

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
  // 转换品牌字符串为IRremoteESP8266协议类型
  if (strcmp(brand, "GREE") == 0)
    return decode_type_t::GREE;
  if (strcmp(brand, "MIDEA") == 0)
    return decode_type_t::MIDEA;
  if (strcmp(brand, "DAIKIN") == 0)
    return decode_type_t::DAIKIN;
  if (strcmp(brand, "HAIER") == 0 || strcmp(brand, "HAIER_AC") == 0)
    return decode_type_t::HAIER_AC;
  if (strcmp(brand, "MITSUBISHI") == 0 || strcmp(brand, "MITSUBISHI_AC") == 0)
    return decode_type_t::MITSUBISHI_AC;
  if (strcmp(brand, "PANASONIC") == 0 || strcmp(brand, "PANASONIC_AC") == 0)
    return decode_type_t::PANASONIC_AC;
  if (strcmp(brand, "SAMSUNG") == 0 || strcmp(brand, "SAMSUNG_AC") == 0)
    return decode_type_t::SAMSUNG_AC;
  if (strcmp(brand, "LG") == 0)
    return decode_type_t::LG;
  if (strcmp(brand, "FUJITSU") == 0 || strcmp(brand, "FUJITSU_AC") == 0)
    return decode_type_t::FUJITSU_AC;
  if (strcmp(brand, "TCL") == 0 || strcmp(brand, "TCL112AC") == 0)
    return decode_type_t::TCL112AC;
  if (strcmp(brand, "COOLIX") == 0)
    return decode_type_t::COOLIX;
  if (strcmp(brand, "TOSHIBA") == 0 || strcmp(brand, "TOSHIBA_AC") == 0)
    return decode_type_t::TOSHIBA_AC;
  if (strcmp(brand, "WHIRLPOOL") == 0 || strcmp(brand, "WHIRLPOOL_AC") == 0)
    return decode_type_t::WHIRLPOOL_AC;
  if (strcmp(brand, "TECO") == 0)
    return decode_type_t::TECO;
  if (strcmp(brand, "SHARP") == 0 || strcmp(brand, "SHARP_AC") == 0)
    return decode_type_t::SHARP_AC;
  if (strcmp(brand, "HITACHI") == 0 || strcmp(brand, "HITACHI_AC") == 0)
    return decode_type_t::HITACHI_AC;
  if (strcmp(brand, "ELECTRA") == 0 || strcmp(brand, "ELECTRA_AC") == 0)
    return decode_type_t::ELECTRA_AC;
  if (strcmp(brand, "CARRIER") == 0 || strcmp(brand, "CARRIER_AC") == 0)
    return decode_type_t::CARRIER_AC;
  if (strcmp(brand, "CORONA") == 0 || strcmp(brand, "CORONA_AC") == 0)
    return decode_type_t::CORONA_AC;
  if (strcmp(brand, "KELON") == 0)
    return decode_type_t::KELON;
  if (strcmp(brand, "KELVINATOR") == 0)
    return decode_type_t::KELVINATOR;
  if (strcmp(brand, "NEOCLIMA") == 0)
    return decode_type_t::NEOCLIMA;
  if (strcmp(brand, "ARGO") == 0)
    return decode_type_t::ARGO;
  if (strcmp(brand, "GOODWEATHER") == 0)
    return decode_type_t::GOODWEATHER;
  if (strcmp(brand, "AMCOR") == 0)
    return decode_type_t::AMCOR;
  if (strcmp(brand, "AIRWELL") == 0)
    return decode_type_t::AIRWELL;
  if (strcmp(brand, "VESTEL") == 0 || strcmp(brand, "VESTEL_AC") == 0)
    return decode_type_t::VESTEL_AC;
  if (strcmp(brand, "VOLTAS") == 0)
    return decode_type_t::VOLTAS;
  if (strcmp(brand, "YORK") == 0)
    return decode_type_t::YORK;

  DEBUG_PRINTF("[红外] ⚠️ 未知品牌: %s\n", brand);
  return decode_type_t::UNKNOWN;
}
