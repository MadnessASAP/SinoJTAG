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
} Mode{};

inline constexpr struct : jtag::Tap::InstructionSet {
  uint8_t Control = 0x2;
  uint8_t Data = 0x3;
  uint8_t Exit = 0xC;
} Instruction{};

/** Enter the SinoWealth diagnostic/special mode before JTAG GPIO init. */
void diag_enter();

/** Transition from diagnostic mode into JTAG mode (mode byte + short reset). */
void jtag_enter();

/** Program SinoWealth JTAG control/data registers after JTAG entry. */
void postinit(Tap& tap);

}  // namespace sinowealth
}  // namespace jtag
