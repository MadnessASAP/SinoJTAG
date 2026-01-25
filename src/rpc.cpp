/*
 * Copyright (C) 2026 Michael "ASAP" Weinrich
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// #include <stdlib.h>
// #include <stddef.h>
#include "rpc.h"

#include <Arduino.h>
// #include <stdint.h>
#include <simpleRPC.h>
// #include <SimpleJTAG/tap.h>

// #include "flash.h"
#include "sinowealth/tap.h"
#include "sinowealth/phy.h"
#include "sinowealth/icp.h"
// #include "vector.tcc"
// #include "icp.h"

#ifndef UART_BAUD
#define UART_BAUD 115200UL
#endif

using namespace SimpleJTAG;
extern sinowealth::Tap _tap;
extern sinowealth::Phy _phy;
extern sinowealth::ICP _icp;

namespace phy {
  void init() {
    _phy.init();
  }
}

namespace tap {

void init() {
  _phy.mode(sinowealth::Phy::Mode::JTAG);
  _tap.init();
}

uint8_t state() {
  return static_cast<uint8_t>(_tap.state());
}

void reset() { _tap.reset(); }

void goto_state(uint8_t target) {
  _tap.goto_state(
      static_cast<Tap::State>(target));
}

uint8_t ir(uint8_t out) {
  uint8_t in = 0;
  _tap.IR(out, &in);
  return in;
}

uint32_t dr(uint32_t out, uint8_t bits) {
  uint32_t in = 0;
  switch (bits) {
    case 4:
      _tap.DR<4, uint32_t>(out, &in);
      break;
    case 8:
      _tap.DR<8, uint32_t>(out, &in);
      break;
    case 16:
      _tap.DR<16, uint32_t>(out, &in);
      break;
    case 23:
      _tap.DR<23, uint32_t>(out, &in);
      break;
    case 32:
      _tap.DR<32, uint32_t>(out, &in);
      break;
    default:
      break;
  }
  return in;
}

void bypass() { _tap.bypass(); }
uint32_t idcode() { return _tap.idcode(); }
void idle_clocks(uint8_t count) { _tap.idle_clocks(count); }

}  // namespace tap

namespace icp {

void init() {
  _phy.mode(sinowealth::Phy::Mode::ICP);
  _icp.init();
}

bool verify() {
  return _icp.verify();
}

Vector<uint8_t> read(uint16_t address, uint8_t size) {
  if (_phy.mode() != sinowealth::Phy::Mode::READY) { _phy.reset(); }
  _phy.mode(sinowealth::Phy::Mode::ICP);
  _icp.init();

  uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
  _icp.read_flash(address, buffer, size);
  _phy.reset();
  return Vector(size, buffer, true);
}

}  // namespace icp

namespace rpc {

void setup() { Serial.begin(UART_BAUD); }

void loop() {
  interface(
      Serial,
      phy::init,
        F("phy_init: Initialize SinoWealth diagnostics mode."),
      tap::init,
        F("tap_init: Initialize JTAG interface."),
      tap::state,
        F("tap_state: Get current TAP state. @return: State (0-15)."),
      tap::reset,
        F("tap_reset: Force TAP to Test-Logic-Reset."),
      tap::goto_state,
        F("tap_goto_state: Move to target state. @target: State (0-15)."),
      tap::ir,
        F("tap_ir: Shift instruction register. @out: Value. @return: Captured."),
      tap::dr,
        F("tap_dr: Shift data register. @out: Value. @bits: Width. @return: Captured."),
      tap::bypass,
        F("tap_bypass: Select BYPASS register."),
      tap::idcode,
        F("tap_idcode: Read IDCODE. @return: 16-bit ID."),
      tap::idle_clocks,
        F("tap_idle_clocks: Emit idle clocks. @count: Number."),
      // flash::read,
      //   F("flash_read: Read 128 bytes from flash. @address: 16-bit addr. @return: [Byte]."),
      icp::init,
        F("icp_init: Initialize ICP interface."),
      icp::verify,
        F("icp_verify: Perform readback test on ICP. @return: Okay"),
      icp::read,
        F("icp_read: Read flash memory via ICP. @address: 16-bit address. @size: 8-bit read length. @return: [Byte]")
  );
}

// namespace flash {
// Vector<uint8_t> read(uint16_t address) {
//   auto buffer = Vector<uint8_t>(128);
//   auto iter = Iterator(_tap, address);
//   for (auto i = 0; i < 128; i++)
//   {
//       buffer[i] = *iter;
//       ++iter;
//   }
//   return buffer;
// }
// }  // namespace flash

}  // namespace rpc
