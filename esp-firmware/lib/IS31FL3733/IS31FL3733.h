/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Neil Enns. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license
 * information.
 *--------------------------------------------------------------------------------------------*/
#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace IS31FL3733 {
const uint8_t CS_LINES = 16; ///< Number of CS lines on the chip.
const uint8_t SW_LINES = 12; ///< Number of SW lines on the chip.
const uint8_t LED_COUNT =
    CS_LINES * SW_LINES; ///< Total number of LEDs in the matrix.

/// @brief Addresses for the common registers.
enum COMMONREGISTER {
  PSR = 0xFD,  ///< Common: page select register. Write only.
  PSWL = 0xFE, ///< Common: page select register write lock. Read/write.
  IMR = 0xF0,  ///< Common: interrupt mask register. Write only.
  ISR = 0xF1,  ///< Common: interrupt status register. Read only.
};

/// @brief Addresses for the paged registers. The high byte is the page
/// for the register. The low byte is the register address.
enum PAGEDREGISTER {
  LEDONOFF =
      0x0000, ///< Page 0: On or off state control for each LED. Write only.
  LEDOPEN = 0x0018,  ///< Page 0: Open state for each LED. Read only.
  LEDSHORT = 0x0030, ///< Page 0: Short state for each LED. Read only.
  LEDPWM = 0x0100,   ///< Page 1: PWM duty cycle for each LED. Write only.
  LEDABM = 0x0200,   ///< Page 2: Auto breath mode for each LED. Write only.
  CR = 0x0300,       ///< Page 3: Configuration register. Write only.
  GCC = 0x0301,      ///< Page 3: Global current control register. Write only.
  ABM1CR =
      0x0302, ///< Page 3: Auto breath control register for ABM-1. Write only.
  ABM2CR =
      0x0306, ///< Page 3: Auto breath control register for ABM-2. Write only.
  ABM3CR =
      0x030A,   ///< Page 3: Auto breath control register for ABM-3. Write only.
  TUR = 0x030E, ///< Page 3: Time update register. Write only.
  SWPUR =
      0x030F, ///< Page 3: SWy pull-up resistor selection register. Write only.
  CSPDR =
      0x0310, ///< Page 3: CSx pull-down resistor selection register. Read only.
  RESET = 0x0311, ///< Page 3: Reset register. Read only.
};

/// @brief Bits to control the PSWL register options.
enum PSWL_OPTIONS {
  PSWL_DISABLE = 0x00, ///< Disable write to page select register.
  PSWL_ENABLE = 0xC5   ///< Enable write to page select register.
};

/// @brief Bits to control the IMR register options.
enum IMR_OPTIONS {
  IMR_IAC = 0x08, ///< Auto clear interrupt bit.
  IMR_IAB = 0x04, ///< Auto breath interrupt bit.
  IMR_IS = 0x02,  ///< Dot short interrupt bit.
  IMR_IO = 0x01,  ///< Dot open interrupt bit.
};

/// @brief Bits to control the ISR register options.
enum ISR_OPTIONS {
  ISR_ABM3 = 0x10, ///< Auto breath mode 3 finish bit
  ISR_ABM2 = 0x08, ///< Auto Breath Mode 2 Finish Bit.
  ISR_ABM1 = 0x04, ///< Auto Breath Mode 1 Finish Bit.
  ISR_SB = 0x02,   ///< Short bit.
  ISR_OB = 0x01,   ///< Open bit.
};

/// @brief Bits to control the CR register options.
enum CR_OPTIONS {
  CR_SYNC_MASTER = 0x40, ///< Configure as clock master device.
  CR_SYNC_SLAVE = 0x80,  ///< Configure as clock slave device.
  CR_OSD = 0x04,         ///< Open/short detection enable bit.
  CR_BEN = 0x02,         ///< Auto breath mode enable bit.
  CR_SSD = 0x01,         ///< Software shutdown bit.
};

/// @brief The valid connection points for ADDR1 and ADDR2 pins.
enum ADDR {
  GND = 0x00, //< Pin connected to GND.
  SCL = 0x01, //< Pin connected to SCL.
  SDA = 0x02, //< Pin connected to SDA.
  VCC = 0x03  //< Pin connected to VCC.
};

/// @brief Values for setting an LED off and on.
enum LED_STATE {
  OFF = 0x00, //< LED is off.
  ON = 0x01   //< LED is on.
};

/// @brief The valid LED status states.
enum LED_STATUS {
  NORMAL = 0x00, //< Normal LED status.
  OPEN = 0x01,   //< LED is open.
  SHORT = 0x02,  //< LED is short.
  UNKNOWN = 0x03 //< Unknown LED status.
};

/// @brief Pull-up or pull-down resistor values.
enum RESISTOR {
  RESISTOR_OFF = 0x00, //< No resistor.
  RESISTOR_500 = 0x01, //< 0.5 kOhm pull-up resistor.
  RESISTOR_1K = 0x02,  //< 1.0 kOhm pull-up resistor.
  RESISTOR_2K = 0x03,  //< 2.0 kOhm pull-up resistor.
  RESISTOR_4K = 0x04,  //< 4.0 kOhm pull-up resistor.
  RESISTOR_8K = 0x05,  //< 8.0 kOhm pull-up resistor.
  RESISTOR_16K = 0x06, //< 16 kOhm pull-up resistor.
  RESISTOR_32K = 0x07  //< 32 kOhm pull-up resistor.
};

/// @brief Maximum number of ABM loop times.
const int ABM_LOOP_TIMES_MAX = 0x0FFF;

/// @brief Loop ABM forever.
const int ABM_LOOP_FOREVER = 0x0000;

/// @brief Configures the LED mode when using ABM.
enum LED_MODE {
  PWM = 0x00,  ///< PWM control mode.
  ABM1 = 0x01, ///< Auto Breath Mode 1.
  ABM2 = 0x02, ///< Auto Breath Mode 2.
  ABM3 = 0x03  ///< Auto Breath Mode 3.
};

/// @brief ABM T1 period time in milliseconds.
enum ABM_T1 {
  T1_210MS = 0x00,
  T1_420MS = 0x20,
  T1_840MS = 0x40,
  T1_1680MS = 0x60,
  T1_3360MS = 0x80,
  T1_6720MS = 0xA0,
  T1_13440MS = 0xC0,
  T1_26880MS = 0xE0
};

/// @brief ABM T2 period time in milliseconds.
enum ABM_T2 {
  T2_0MS = 0x00,
  T2_210MS = 0x02,
  T2_420MS = 0x04,
  T2_840MS = 0x06,
  T2_1680MS = 0x08,
  T2_3360MS = 0x0A,
  T2_6720MS = 0x0C,
  T2_13440MS = 0x0E,
  T2_26880MS = 0x10
};

/// @brief ABM T3 period time in milliseconds.
enum ABM_T3 {
  T3_210MS = 0x00,
  T3_420MS = 0x20,
  T3_840MS = 0x40,
  T3_1680MS = 0x60,
  T3_3360MS = 0x80,
  T3_6720MS = 0xA0,
  T3_13440MS = 0xC0,
  T3_26880MS = 0xE0
};

/// @brief ABM T4 period time in milliseconds.
enum ABM_T4 {
  T4_0MS = 0x00,
  T4_210MS = 0x02,
  T4_420MS = 0x04,
  T4_840MS = 0x06,
  T4_1680MS = 0x08,
  T4_3360MS = 0x0A,
  T4_6720MS = 0x0C,
  T4_13440MS = 0x0E,
  T4_26880MS = 0x10,
  T4_53760MS = 0x12,
  T4_107520MS = 0x14
};

/// @brief ABM loop beginning time.
enum ABM_LOOP_BEGIN {
  LOOP_BEGIN_T1 = 0x00,
  LOOP_BEGIN_T2 = 0x10,
  LOOP_BEGIN_T3 = 0x20,
  LOOP_BEGIN_T4 = 0x30
};

/// @brief ABM loop end time.
enum ABM_LOOP_END { LOOP_END_T3 = 0x00, LOOP_END_T1 = 0x40 };

/// @brief ABM function number, also used as register offset.
enum ABM_NUM {
  NUM_1 = PAGEDREGISTER::ABM1CR,
  NUM_2 = PAGEDREGISTER::ABM2CR,
  NUM_3 = PAGEDREGISTER::ABM3CR
};

/// @brief Structure for providing ABM configuration options.
struct ABM_CONFIG {
  ABM_T1 T1;
  ABM_T2 T2;
  ABM_T3 T3;
  ABM_T4 T4;
  ABM_LOOP_BEGIN Tbegin;
  ABM_LOOP_END Tend;
  uint16_t Times;
};

/// @brief Function definition for reading and writing the registers.
typedef uint8_t (*i2c_read_function)(const uint8_t i2c_addr,
                                     const uint8_t reg_addr, uint8_t *buffer,
                                     const uint8_t count);
typedef uint8_t (*i2c_write_function)(const uint8_t i2c_addr,
                                      const uint8_t reg_addr,
                                      const uint8_t *buffer,
                                      const uint8_t count);

/// @brief Driver for interacting with IS31FL3733 chips.
class IS31FL3733Driver {
private:
  const uint8_t I2C_BASE_ADDR = 0xA0;
  uint8_t maxI2CWriteBufferSize;

  uint8_t address;
  i2c_read_function i2c_read_reg;
  i2c_write_function i2c_write_reg;
  uint8_t leds[SW_LINES * CS_LINES / 8];

  void _setColumnPagedRegister(const PAGEDREGISTER reg, const uint8_t cs,
                               uint8_t value);
  void _setFullPagedRegister(const PAGEDREGISTER reg, const uint8_t value);
  void _setRowPagedRegister(const PAGEDREGISTER reg, const uint8_t sw,
                            uint8_t value);
  void _setLEDState(const uint8_t offset, const uint8_t cs,
                    const LED_STATE state);

public:
  IS31FL3733Driver(const ADDR addr1, const ADDR addr2,
                   const i2c_read_function read_function,
                   const i2c_write_function write_function);

  byte GetI2CAddress();

  uint8_t ReadCommonReg(const COMMONREGISTER reg);
  void WriteCommonReg(const COMMONREGISTER reg, const uint8_t reg_value);
  void SelectPageForRegister(const PAGEDREGISTER reg);
  uint8_t ReadPagedReg(const PAGEDREGISTER reg);
  uint8_t ReadPagedReg(const PAGEDREGISTER reg, const uint8_t offset);
  void WritePagedReg(const PAGEDREGISTER reg, const uint8_t reg_value);
  void WritePagedReg(const PAGEDREGISTER reg, const uint8_t offset,
                     const uint8_t reg_value);
  void WritePagedRegs(const PAGEDREGISTER reg, const uint8_t *values,
                      const uint8_t count);
  void WritePagedRegs(const PAGEDREGISTER reg, const uint8_t offset,
                      const uint8_t *values, const uint8_t count);

  void Init();
  void SetGCC(const uint8_t gcc);
  void SetSWPUR(const RESISTOR resistor);
  void SetCSPDR(const RESISTOR resistor);

  void SetLEDSingleState(const uint8_t cs, const uint8_t sw,
                         const LED_STATE state);
  void SetLEDColumnState(uint8_t cs, const LED_STATE state);
  void SetLEDRowState(uint8_t sw, const LED_STATE state);
  void SetLEDMatrixState(const LED_STATE state);

  void SetLEDSinglePWM(const uint8_t cs, const uint8_t sw, const uint8_t value);
  void SetLEDColumnPWM(const uint8_t cs, const uint8_t value);
  void SetLEDRowPWM(const uint8_t sw, const uint8_t value);
  void SetLEDMatrixPWM(const uint8_t value);

  void SetState(const LED_STATE *states);
  void SetPWM(const uint8_t *values);

  LED_STATUS GetLEDStatus(const uint8_t cs, const uint8_t sw);

  void SetLEDSingleMode(uint8_t cs, uint8_t sw, const LED_MODE mode);
  void SetLEDRowMode(const uint8_t sw, const LED_MODE mode);
  void SetLEDColumnMode(const uint8_t cs, const LED_MODE mode);
  void SetLEDMatrixMode(const LED_MODE mode);

  void ConfigABM(const ABM_NUM n, const ABM_CONFIG *config);
  void StartABM();
};

} // namespace IS31FL3733
