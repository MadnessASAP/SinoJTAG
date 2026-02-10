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
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>

#include "config.h"

namespace SimpleJTAG {

/** Stateless JTAG PHY bit-banging implementation. */
struct Phy {
  /**
   * @pre config::tck/tms/tdi/tdo pins are valid.
   * @post TCK/TMS/TDI outputs, TDO input; TCK low, TMS high, TDI low.
   */
  static inline void init() {
    set_ddr(config::tck::ddr, config::tck::index, true);
    set_ddr(config::tms::ddr, config::tms::index, true);
    set_ddr(config::tdi::ddr, config::tdi::index, true);
    set_ddr(config::tdo::ddr, config::tdo::index, false);

    write_port(config::tdo::port, config::tdo::index, config::tdo_pullup);
    write_port(config::tck::port, config::tck::index, false);
    write_port(config::tms::port, config::tms::index, true);
    write_port(config::tdi::port, config::tdi::index, false);
  }

  /**Reset IO to Hi-Z pullups off */
  static inline void stop() {
    set_ddr(config::tck::ddr, config::tck::index, false);
    set_ddr(config::tms::ddr, config::tms::index, false);
    set_ddr(config::tdi::ddr, config::tdi::index, false);
    set_ddr(config::tdo::ddr, config::tdo::index, false);

    write_port(config::tdo::port, config::tdo::index, false);
    write_port(config::tck::port, config::tck::index, false);
    write_port(config::tms::port, config::tms::index, false);
    write_port(config::tdi::port, config::tdi::index, false);
  }

  /**
   * @pre init() completed and JTAG pins configured.
   * @post One TCK pulse applied with given TMS.
   */
  static inline void next_state(bool tms) {
    write_port(config::tms::port, config::tms::index, tms);
    pulse_tck();
  }

  /**
   * @pre init() completed; pins configured for JTAG.
   * @post Shifted bits LSB-first; optional capture stored in @p in.
   */
  static inline uint32_t stream_bits(uint32_t out, uint8_t bits, bool exit,
                                     uint32_t *in = nullptr) {
    if (bits == 0) {
      if (in) {
        *in = 0;
      }
      return 0;
    }

    uint32_t capture = 0;
    for (uint8_t i = 0; i < bits; ++i) {
      // setup TMS and TDI
      const bool is_last = (i + 1) == bits;
      write_port(config::tms::port, config::tms::index, exit && is_last);
      write_port(config::tdi::port, config::tdi::index, (out & 0x1u) != 0);

      // clock out TMS and TDI
      set_tck(false);
      config::delay_half();

      // Clock in TDO
      set_tck(true);
      config::delay_half();

      // Read TDO
      if (read_pin(config::tdo::pin, config::tdo::index)) {
        if (i < 32) {
          capture |= (1UL << i);
        }
      }

      // reset clock and advance bit
      set_tck(false);
      out >>= 1;
    }

    if (in) {
      *in = capture;
    }
    return capture;
  }

  /** Convenience wrapper for compile-time-sized shifts. */
  template <int N, bool EXIT, typename T>
  static inline T stream_bits(T out, T *in = nullptr) {
    static_assert(N > 0, "stream_bits N must be >= 1");

    T capture = 0;
    for (uint8_t i = 0; i < N; ++i) {
      const bool is_last = (i + 1) == N;
      write_port(config::tms::port, config::tms::index, EXIT && is_last);
      write_port(config::tdi::port, config::tdi::index, (out & T(1)) != 0);

      set_tck(false);
      config::delay_half();
      set_tck(true);
      config::delay_half();

      if (read_pin(config::tdo::pin, config::tdo::index)) {
        capture |= (T(1) << i);
      }

      set_tck(false);
      out >>= 1;
    }

    if (in) {
      *in = capture;
    }
    return capture;
  }

  /** Configure GPIO direction bit. */
  static inline void set_ddr(volatile uint8_t& ddr, uint8_t bit, bool output) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    if (output) {
      ddr |= mask;
    } else {
      ddr &= static_cast<uint8_t>(~mask);
    }
  }

  /** Write GPIO output bit. */
  static inline void write_port(volatile uint8_t& port, uint8_t bit, bool value) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    if (value) {
      port |= mask;
    } else {
      port &= static_cast<uint8_t>(~mask);
    }
  }

  /** Read GPIO input bit. */
  static inline bool read_pin(volatile uint8_t& pin, uint8_t bit) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    return (pin & mask) != 0;
  }

 private:
  /** Pulse TCK low->high->low with timing delays. */
  static inline void pulse_tck() {
    set_tck(false);
    config::delay_half();
    set_tck(true);
    config::delay_half();
    set_tck(false);
  }

  /** Drive TCK output. */
  static inline void set_tck(bool value) {
    write_port(config::tck::port, config::tck::index, value);
  }
};

} // namespace SimpleJTAG
