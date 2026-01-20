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

#include <stdbool.h>
#include <stdint.h>

namespace jtag {

/** Function table for non-templated PHY access. */
struct IPHYIface {
  /** Optional pre-init hook before any GPIO configuration. */
  void (*preinit)();
  /** Configure GPIO direction and idle levels for JTAG. */
  void (*init)();
  /** Drive TMS and pulse TCK once. */
  void (*next_state)(bool tms);
  /** Shift bits LSB-first and optionally capture TDO. */
  uint32_t (*stream_bits)(uint32_t out, uint8_t bits, bool exit, uint32_t* in);
};

/** Invoke the optional pre-init hook before GPIO configuration. */
static inline void iphy_preinit(const IPHYIface& iface) {
  if (iface.preinit) {
    iface.preinit();
  }
}

/** Configure JTAG GPIO directions and idle levels. */
static inline void iphy_init(const IPHYIface& iface) {
  iface.init();
}

/** Drive TMS for one clock to advance the TAP state. */
static inline void iphy_next_state(const IPHYIface& iface, bool tms) {
  iface.next_state(tms);
}

/** Shift up to 32 bits LSB-first and optionally capture TDO. */
static inline uint32_t iphy_stream_bits(const IPHYIface& iface,
                                        uint32_t out,
                                        uint8_t bits,
                                        bool exit,
                                        uint32_t* in) {
  return iface.stream_bits(out, bits, exit, in);
}

}  // namespace jtag
