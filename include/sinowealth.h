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

#include <stdint.h>

#include "tap.h"

namespace jtag {
namespace sinowealth {

inline constexpr struct {
  uint8_t JTAG = 0xA5;
  uint8_t ICP = 0x69;
} Mode{};

inline constexpr struct : jtag::Tap::InstructionSet {
  uint8_t Control = 0x2;
  uint8_t Data = 0x3;
  uint8_t Exit = 0xC;
} Instruction{};

// Some data is sent MSB, these functions do a bitorder flip

/** Flip the bits of a uint16_t */
inline constexpr uint16_t reverse16(uint16_t v) {
  v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
  v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
  v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
  v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
  return v;
}

/** Flip the bits of a uint8_t */
inline constexpr uint8_t reverse8(uint8_t v) {
  v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0);
  v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC);
  v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA);
  return v;
}

/** Enter the SinoWealth diagnostic/special mode before JTAG GPIO init. */
void diag_enter();

/** Transition from diagnostic mode into JTAG mode */
void jtag_enter();

/** Transition from diagnostic mode into ICP mode */
void icp_enter();

/** Transition from diagnostic mode into specified mode (mode byte + init clocks).\
 * mode byte is sent LSB first
 */
void mode_enter(uint8_t mode);

/** Program SinoWealth JTAG control/data registers after JTAG entry. */
void postinit(Tap& tap);

}  // namespace sinowealth
}  // namespace jtag
