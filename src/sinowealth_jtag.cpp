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

#include "sinowealth.h"
#include "tap_state.h"

#include <util/delay.h>

namespace jtag {
namespace sinowealth {

/** Program SinoWealth JTAG control/data registers after JTAG entry. */
void postinit(Tap& tap) {
  tap.goto_state(jtag::State::RunTestIdle);
  tap.idle_clocks(2);

  tap.IR(Instruction.Control);
  tap.DR<4>(4);           // TODO: What does this mean?
  tap.idle_clocks(1);

  tap.IR(Instruction.Data);
  tap.DR<23>(0x403000u);  // TODO: What does this mean?
  tap.idle_clocks(1); // Run-Test/Idle
  _delay_us(50);          // TODO: Is this neccessary?
  tap.DR<23>(0x402000u);  // TODO: What does this mean?
  tap.idle_clocks(1); // Run-Test/Idle
  tap.DR<23>(0x400000u);  // TODO: What does this mean?
  tap.idle_clocks(1); // Run-Test/Idle

  // TODO: Is this neccessary?
  const uint32_t breakpoints[] = {
      0x630000u, 0x670000u, 0x6B0000u, 0x6F0000u,
      0x730000u, 0x770000u, 0x7B0000u, 0x7F0000u,
  };
  for (uint8_t i = 0; i < (sizeof(breakpoints) / sizeof(breakpoints[0])); ++i) {
    tap.DR<23, uint32_t>(breakpoints[i], nullptr);
    tap.idle_clocks(1);
  }

  tap.IR(Instruction.Control);
  tap.DR<4>(1);           // TODO: What does this mean?
  tap.idle_clocks(1); // Run-Test/Idle

  tap.IR(Instruction.Exit);
}

}  // namespace sinowealth
}  // namespace jtag
