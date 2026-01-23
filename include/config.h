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

#pragma once

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

namespace config {

#define DEFINE_PIN(NAME, PORT_LETTER, BIT)                     \
  namespace NAME {                                             \
    static inline volatile uint8_t& port = PORT##PORT_LETTER;  \
    static inline volatile uint8_t& ddr = DDR##PORT_LETTER;    \
    static inline volatile uint8_t& pin = PIN##PORT_LETTER;    \
    static constexpr uint8_t index = BIT;                      \
  }

DEFINE_PIN(tck, D, 5);
DEFINE_PIN(tms, D, 3);
DEFINE_PIN(tdi, D, 4);
DEFINE_PIN(tdo, D, 2);
DEFINE_PIN(vref, D, 6);

#undef DEFINE_PIN

/** Enable pull-up on TDO input (set false if target drives push-pull). */
static constexpr bool tdo_pullup = true;

/** Delay half-period for TCK (~250 kHz at 16 MHz F_CPU). */
static inline void delay_half() { _delay_us(1); }

}  // namespace config
