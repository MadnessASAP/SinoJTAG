/*
 * This file is derived from work done by Michal Kovacik at:
 * https://github.com/gashtaan/sinowealth-8051-dumpe/dumper/jtag.cpp
 *
 * Copyright (C) 2023, Michal Kovacik
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

#include "config.h"
#include "phy.h"
#include "sinowealth.h"
#include <util/delay.h>

namespace jtag {
namespace sinowealth {

/** Send the SinoWealth mode byte with extra trailing clocks. */
static inline void send_mode_byte(uint8_t mode) {
  Phy::stream_bits<8, false>(mode);
  Phy::next_state(false);
  Phy::next_state(false);
}

/** Drive TCK output during pre-init waveform. */
static inline void drive_tck(bool value) {
  Phy::write_port(config::tck::port, config::tck::index, value);
}

/** Drive TMS output during pre-init waveform. */
static inline void drive_tms(bool value) {
  Phy::write_port(config::tms::port, config::tms::index, value);
}

/** Drive TDI output during pre-init waveform. */
static inline void drive_tdi(bool value) {
  Phy::write_port(config::tdi::port, config::tdi::index, value);
}

/** Read the target Vref sense input. */
static inline bool read_vref() {
  return Phy::read_pin(config::vref::pin, config::vref::index);
}


/** Enter the SinoWealth diagnostic/special mode before JTAG GPIO init. */
void diag_enter() {
  // Block on Vref, flash LED on PB5 to signal waiting
  Phy::set_ddr(config::vref::ddr, config::vref::index, false);    // input
  Phy::write_port(config::vref::port, config::vref::index, false); // no pull-up

  DDRB |= _BV(5);   // PB5 output (LED)
  uint8_t count = 0;
  while (!read_vref()) {
    if (++count == 0) {
      PORTB ^= _BV(5);  // Toggle LED every 256 iterations
    }
    _delay_us(200);
  }
  PORTB &= ~_BV(5);  // LED off

  // Enable outputs
  Phy::set_ddr(config::tck::ddr, config::tck::index, true);
  Phy::set_ddr(config::tdi::ddr, config::tdi::index, true);
  Phy::set_ddr(config::tms::ddr, config::tms::index, true);
  drive_tck(true);
  drive_tdi(true);
  drive_tms(true);

  _delay_us(500);
  drive_tck(false);
  _delay_us(1);
  drive_tck(true);
  _delay_us(50);

  for (uint8_t n = 0; n < 165; ++n) {
    drive_tms(false);
    _delay_us(2);
    drive_tms(true);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 105; ++n) {
    drive_tdi(false);
    _delay_us(2);
    drive_tdi(true);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 90; ++n) {
    drive_tck(false);
    _delay_us(2);
    drive_tck(true);
    _delay_us(2);
  }

  for (uint16_t n = 0; n < 25600; ++n) {
    drive_tms(false);
    _delay_us(2);
    drive_tms(true);
    _delay_us(2);
  }

  _delay_us(8);
  drive_tms(false);
}

/** Transition from diagnostic mode into JTAG mode (mode byte + short reset). */
void jtag_enter() {
  send_mode_byte(Mode.JTAG);
  for (uint8_t n = 0; n < 8; ++n) {
    Phy::next_state(true);
  }
  Phy::next_state(false);
  Phy::next_state(false);
}

}  // namespace sinowealth
}  // namespace jtag
