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

#include "board_pins.h"
#include "phy.h"
#include "sinowealth.h"
#include <util/delay.h>

namespace jtag {
namespace sinowealth {

using cfg = jtag::Config;
using PrePhy = jtag::Phy<jtag::Config, jtag::Timing>;

/** Send the SinoWealth mode byte with extra trailing clocks. */
static inline void send_mode_byte(const IPHYIface& iface, uint8_t mode) {
  iphy_stream_bits(iface, mode, 8, false, nullptr);
  iface.next_state(false);
  iface.next_state(false);
}

/** Drive TCK output during pre-init waveform. */
static inline void drive_tck(bool value) {
  PrePhy::write_port(cfg::tck_port(), cfg::tck_bit, value);
}

/** Drive TMS output during pre-init waveform. */
static inline void drive_tms(bool value) {
  PrePhy::write_port(cfg::tms_port(), cfg::tms_bit, value);
}

/** Drive TDI output during pre-init waveform. */
static inline void drive_tdi(bool value) {
  PrePhy::write_port(cfg::tdi_port(), cfg::tdi_bit, value);
}

/** Read the target Vref sense input. */
static inline bool read_vref() {
  return PrePhy::read_pin(cfg::vref_pin(), cfg::vref_bit);
}


void diag_enter() {
  // Block on Vref
  PrePhy::set_ddr(cfg::vref_ddr(), cfg::vref_bit, false);   // input
  PrePhy::write_port(cfg::vref_port(), cfg::vref_bit, false); // no pull-up
  while (!read_vref()) {
  }

  // Enable outputs
  PrePhy::set_ddr(cfg::tck_ddr(), cfg::tck_bit, true);
  PrePhy::set_ddr(cfg::tdi_ddr(), cfg::tdi_bit, true);
  PrePhy::set_ddr(cfg::tms_ddr(), cfg::tms_bit, true);
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

void jtag_enter(const IPHYIface& iface) {
  static constexpr uint8_t kModeJtag = 0xA5u;

  send_mode_byte(iface, kModeJtag);
  for (uint8_t n = 0; n < 8; ++n) {
    iface.next_state(true);
  }
  iface.next_state(false);
  iface.next_state(false);
}

}  // namespace sinowealth
}  // namespace jtag
