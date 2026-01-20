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
#include <avr/sfr_defs.h>
#include <util/delay.h>

using cfg = jtag::Config;

#define set(r, b) *r |= _BV(b)
#define clr(r, b) *r &= ~_BV(b)

#define tck(v)                                                                 \
  if constexpr (v) {                                                           \
    set(cfg::tck_port(), cfg::tck_bit);                                        \
  } else {                                                                     \
    clr(cfg::tck_port(), cfg::tck_bit);                                        \
  }

#define tms(v)                                                                 \
  if constexpr (v) {                                                           \
    set(cfg::tms_port(), cfg::tms_bit);                                        \
  } else {                                                                     \
    clr(cfg::tms_port(), cfg::tms_bit);                                        \
  }

#define tdi(v)                                                                 \
  if constexpr (v) {                                                           \
    set(cfg::tdi_port(), cfg::tdi_bit);                                        \
  } else {                                                                     \
    clr(cfg::tdi_port(), cfg::tdi_bit);                                        \
  }

#define vref (*cfg::vref_pin() & _BV(cfg::vref_bit))


void jtag::preinit() {
  // Block on Vref
  clr(cfg::vref_ddr(), cfg::vref_bit);  // input
  clr(cfg::vref_port(), cfg::vref_bit); // no pull-up
  while (!vref) {
  }

  // Enable outputs
  *cfg::tck_ddr() |= _BV(cfg::tck_bit);
  *cfg::tdi_ddr() |= _BV(cfg::tdi_bit);
  *cfg::tms_ddr() |= _BV(cfg::tms_bit);
  tck(1);
  tdi(1);
  tms(1);

  _delay_us(500);
  tck(0);
  _delay_us(1);
  tck(1);
  _delay_us(50);

  for (uint8_t n = 0; n < 165; ++n) {
    tms(0);
    _delay_us(2);
    tms(1);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 105; ++n) {
    tdi(0);
    _delay_us(2);
    tdi(1);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 90; ++n) {
    tck(0);
    _delay_us(2);
    tck(1);
    _delay_us(2);
  }

  for (uint16_t n = 0; n < 25600; ++n) {
    tms(0);
    _delay_us(2);
    tms(1);
    _delay_us(2);
  }

  _delay_us(8);
  tms(0);
}
