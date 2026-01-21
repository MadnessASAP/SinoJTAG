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
#include <util/delay.h>
#include <stdint.h>

namespace jtag {

#if 0
/** Placeholder pins definition; replace with target-specific mapping. */
struct PinsUnset {
  /** TCK PORT register. */
  static inline volatile uint8_t* tck_port() { return (volatile uint8_t*)0; }
  /** TCK DDR register. */
  static inline volatile uint8_t* tck_ddr() { return (volatile uint8_t*)0; }
  /** TCK bit position. */
  static constexpr uint8_t tck_bit = 0;

  /** TMS PORT register. */
  static inline volatile uint8_t* tms_port() { return (volatile uint8_t*)0; }
  /** TMS DDR register. */
  static inline volatile uint8_t* tms_ddr() { return (volatile uint8_t*)0; }
  /** TMS bit position. */
  static constexpr uint8_t tms_bit = 0;

  /** TDI PORT register. */
  static inline volatile uint8_t* tdi_port() { return (volatile uint8_t*)0; }
  /** TDI DDR register. */
  static inline volatile uint8_t* tdi_ddr() { return (volatile uint8_t*)0; }
  /** TDI bit position. */
  static constexpr uint8_t tdi_bit = 0;

  /** TDO PORT register (pull-up control). */
  static inline volatile uint8_t* tdo_port() { return (volatile uint8_t*)0; }
  /** TDO DDR register. */
  static inline volatile uint8_t* tdo_ddr() { return (volatile uint8_t*)0; }
  /** TDO PIN register (input read). */
  static inline volatile uint8_t* tdo_pin() { return (volatile uint8_t*)0; }
  /** TDO bit position. */
  static constexpr uint8_t tdo_bit = 0;
};
#endif

/** Active pin mapping for the current target wiring. */
struct Config {
  /** TCK PORT register. */
  static inline volatile uint8_t* tck_port() { return &PORTD; }
  /** TCK DDR register. */
  static inline volatile uint8_t* tck_ddr() { return &DDRD; }
  /** TCK bit position. */
  static constexpr uint8_t tck_bit = 5;

  /** TMS PORT register. */
  static inline volatile uint8_t* tms_port() { return &PORTD; }
  /** TMS DDR register. */
  static inline volatile uint8_t* tms_ddr() { return &DDRD; }
  /** TMS bit position. */
  static constexpr uint8_t tms_bit = 3;

  /** TDI PORT register. */
  static inline volatile uint8_t* tdi_port() { return &PORTD; }
  /** TDI DDR register. */
  static inline volatile uint8_t* tdi_ddr() { return &DDRD; }
  /** TDI bit position. */
  static constexpr uint8_t tdi_bit = 4;

  /** TDO PORT register (pull-up control). */
  static inline volatile uint8_t* tdo_port() { return &PORTD; }
  /** TDO DDR register. */
  static inline volatile uint8_t* tdo_ddr() { return &DDRD; }
  /** TDO PIN register (input read). */
  static inline volatile uint8_t* tdo_pin() { return &PIND; }
  /** TDO bit position. */
  static constexpr uint8_t tdo_bit = 2;
  /** Enable pull-up on TDO input (set false if target drives push-pull). */
  static constexpr bool tdo_pullup = true;

  /** Vref PORT register (pull-up control). */
  static inline volatile uint8_t* vref_port() { return &PORTD; }
  /** Vref DDR register. */
  static inline volatile uint8_t* vref_ddr() { return &DDRD; }
  /** Vref PIN register (input read). */
  static inline volatile uint8_t* vref_pin() { return &PIND; }
  /** Vref bit position. */
  static constexpr uint8_t vref_bit = 6;
};

/** Timing helper that yields ~250 kHz TCK at 16 MHz F_CPU. */
struct Timing {
  /** Delay half-period for TCK. */
  static inline void delay_half() { _delay_us(2); }
};

}  // namespace jtag
