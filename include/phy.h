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

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>

#include "board_pins.h"
#include "iphy.h"

namespace jtag {

/** Stateless JTAG PHY bit-banging implementation. */
template <typename Pins, typename Timing> struct Phy {
  /**
   * @pre GPIOs are still at power-on defaults; no DDR/PORT writes yet.
   * @post Target-specific enable waveform has been applied.
   */
  static inline void preinit() { Pins::preinit(); }

  /**
   * @pre Pins::tck/tms/tdi/tdo pointers and bits are valid.
   * @post TCK/TMS/TDI outputs, TDO input; TCK low, TMS high, TDI low.
   */
  static inline void init() {
    set_ddr(Pins::tck_ddr(), Pins::tck_bit, true);
    set_ddr(Pins::tms_ddr(), Pins::tms_bit, true);
    set_ddr(Pins::tdi_ddr(), Pins::tdi_bit, true);
    set_ddr(Pins::tdo_ddr(), Pins::tdo_bit, false);

    write_port(Pins::tdo_port(), Pins::tdo_bit, false);
    write_port(Pins::tck_port(), Pins::tck_bit, false);
    write_port(Pins::tms_port(), Pins::tms_bit, true);
    write_port(Pins::tdi_port(), Pins::tdi_bit, false);
  }

  /**
   * @pre init() completed and JTAG pins configured.
   * @post One TCK pulse applied with given TMS.
   */
  static inline void next_state(bool tms) {
    write_port(Pins::tms_port(), Pins::tms_bit, tms);
    pulse_tck();
  }

  /**
   * @pre init() completed; pins configured for JTAG.
   * @post Shifted bits LSB-first; optional capture stored in @p in.
   */
  static inline uint32_t stream_bits(uint32_t out, uint8_t bits, bool exit,
                                     uint32_t *in) {
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
      write_port(Pins::tms_port(), Pins::tms_bit, exit && is_last);
      write_port(Pins::tdi_port(), Pins::tdi_bit, (out & 0x1u) != 0);

      // clock out TMS and TDI
      set_tck(false);
      Timing::delay_half();

      // Clock in TDO
      set_tck(true);
      Timing::delay_half();

      // Read TDO
      if (read_pin(Pins::tdo_pin(), Pins::tdo_bit)) {
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
  template <int N, typename T, bool EXIT>
  static inline T stream_bits(T out, T *in = nullptr) {
    static_assert(N > 0 && N <= 32, "stream_bits N must be 1..32");
    static_assert(sizeof(T) <= 4, "stream_bits T must be <= 32 bits");

    // unpack output and send out while capturing
    const uint32_t capture =
        stream_bits(static_cast<uint32_t>(out), N, EXIT, nullptr);

    // pack input and return
    if (in) {
      *in = static_cast<T>(capture);
    }
    return static_cast<T>(capture);
  }

private:
  /** Pulse TCK low->high->low with timing delays. */
  static inline void pulse_tck() {
    set_tck(false);
    Timing::delay_half();
    set_tck(true);
    Timing::delay_half();
    set_tck(false);
  }

  /** Drive TCK output. */
  static inline void set_tck(bool value) {
    write_port(Pins::tck_port(), Pins::tck_bit, value);
  }

  /** Configure GPIO direction bit. */
  static inline void set_ddr(volatile uint8_t *ddr, uint8_t bit, bool output) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    if (output) {
      *ddr |= mask;
    } else {
      *ddr &= static_cast<uint8_t>(~mask);
    }
  }

  /** Write GPIO output bit. */
  static inline void write_port(volatile uint8_t *port, uint8_t bit,
                                bool value) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    if (value) {
      *port |= mask;
    } else {
      *port &= static_cast<uint8_t>(~mask);
    }
  }

  /** Read GPIO input bit. */
  static inline bool read_pin(volatile uint8_t *pin, uint8_t bit) {
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    return ((*pin) & mask) != 0;
  }
};

/** Adapter that exposes a templated PHY through a function table. */
template <typename Pins, typename Timing> struct IPHY {
  /** Return the function table for this PHY specialization. */
  static inline const IPHYIface &iface() {
    static const IPHYIface kIface = {
        &Phy<Pins, Timing>::preinit,
        &Phy<Pins, Timing>::init,
        &Phy<Pins, Timing>::next_state,
        &Phy<Pins, Timing>::stream_bits,
    };
    return kIface;
  }
};

} // namespace jtag
