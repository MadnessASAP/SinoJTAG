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

#include <stddef.h>
#include "rpc.h"

#include <Arduino.h>
#include <stdint.h>
#include <simpleRPC.h>

#include "flash.h"
#include "sinowealth.h"
#include "tap.h"
#include "vector.tcc"

#ifndef JTAG_IR_BITS
#define JTAG_IR_BITS 4
#endif

#ifndef UART_BAUD
#define UART_BAUD 115200UL
#endif

namespace {
jtag::Tap<JTAG_IR_BITS> tap_instance;
}

namespace rpc {

void setup() { Serial.begin(UART_BAUD); }

void loop() {
  interface(
      Serial,
      tap::init,
        F("init: Initialize JTAG interface."),
      tap::state,
        F("state: Get current TAP state. @return: State (0-15)."),
      tap::reset,
        F("reset: Force TAP to Test-Logic-Reset."),
      tap::goto_state,
        F("goto_state: Move to target state. @target: State (0-15)."),
      tap::ir,
        F("ir: Shift instruction register. @out: Value. @return: Captured."),
      tap::dr,
        F("dr: Shift data register. @out: Value. @bits: Width. @return: Captured."),
      tap::bypass,
        F("bypass: Select BYPASS register."),
      tap::idcode,
        F("idcode: Read IDCODE. @return: 16-bit ID."),
      tap::idle_clocks,
        F("idle_clocks: Emit idle clocks. @count: Number."),
      flash::read,
        F("flash_read: Read 128 bytes from flash. @address: 16-bit addr. @return: [Byte].")
  );
}

namespace tap {

void init() {
  jtag::sinowealth::diag_enter();
  tap_instance.init();
  jtag::sinowealth::jtag_enter();
  jtag::sinowealth::postinit(tap_instance);
}

uint8_t state() {
  return static_cast<uint8_t>(tap_instance.state());
}

void reset() { tap_instance.reset(); }

void goto_state(uint8_t target) {
  tap_instance.goto_state(
      static_cast<jtag::Tap<JTAG_IR_BITS>::State>(target));
}

uint8_t ir(uint8_t out) {
  uint8_t in = 0;
  tap_instance.IR<uint8_t>(out, &in);
  return in;
}

uint32_t dr(uint32_t out, uint8_t bits) {
  uint32_t in = 0;
  switch (bits) {
    case 4:
      tap_instance.DR<4, uint32_t>(out, &in);
      break;
    case 8:
      tap_instance.DR<8, uint32_t>(out, &in);
      break;
    case 16:
      tap_instance.DR<16, uint32_t>(out, &in);
      break;
    case 23:
      tap_instance.DR<23, uint32_t>(out, &in);
      break;
    case 32:
      tap_instance.DR<32, uint32_t>(out, &in);
      break;
    default:
      break;
  }
  return in;
}

void bypass() { tap_instance.bypass(); }

uint16_t idcode() {
  uint16_t id = 0;
  tap_instance.idcode<16, uint16_t>(&id);
  return id;
}

void idle_clocks(uint8_t count) { tap_instance.idle_clocks(count); }

}  // namespace tap

namespace flash {
Vector<uint8_t> read(uint16_t address) {
    auto buffer = Vector<uint8_t>(128);
    auto iter = jtag::flash::start(tap_instance, address);
    for (auto i = 0; i < 128; i++)
    {
        buffer[i] = *iter++;
    }
   jtag::flash::end(iter);

   return buffer;
}
}  // namespace flash
}  // namespace rpc
