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

#include <avr/io.h>
#include <stdint.h>

#include "board_pins.h"
#include "phy.h"
#include "serial.h"
#include "sinowealth.h"
#include "tap.h"

#ifndef JTAG_IR_BITS
#define JTAG_IR_BITS 4
#endif

#define JTAG_BRINGUP

using PHY = jtag::IPHY<jtag::Config, jtag::Timing>;

#ifndef UART_BAUD
#define UART_BAUD 115200UL
#endif

/** Entry point for bring-up and firmware loop. */
int main() {
  serial_init(UART_BAUD);
  serial_write_str("SimpleJTAG start\r\n");

#ifdef JTAG_BRINGUP

  const auto& iface = PHY::iface();
  jtag::Tap<JTAG_IR_BITS> tap(iface);

  serial_write_str("DIAG\n");
  jtag::sinowealth::diag_enter();
  serial_write_str("INIT\n");
  tap.init();
  serial_write_str("JTAG ENTER\n");
  jtag::sinowealth::jtag_enter(iface);
  serial_write_str("POSTINIT\n");
  jtag::sinowealth::postinit(tap);

  uint16_t idcode = 0;
  tap.idcode<16, uint16_t>(&idcode);
  serial_write_str("IDCODE: 0x");
  serial_write_hex32(idcode);
  serial_write_str("\r\n");

#endif

  for (;;) {
  }
}
