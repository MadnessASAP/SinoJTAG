/*
 * Copyright (C) 2026 Michael "ASAP" Weinrich
 *
 * With inclusions from https://github.com/gashtaan/sinowealth-8051-dumper
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

#include "sinowealth/tap.h"

namespace sinowealth {

void Tap::init() {
  goto_state(State::RunTestIdle);
  idle_clocks(2);

  IR(InstructionSet::Control);
  DR<4>(4);           // TODO: What does this mean?
  idle_clocks(1);

  IR(InstructionSet::Data);
  DR<23>(0x403000u);  // TODO: What does this mean?
  idle_clocks(1); // Run-Test/Idle
  _delay_us(50);          // TODO: Is this neccessary?
  DR<23>(0x402000u);  // TODO: What does this mean?
  idle_clocks(1); // Run-Test/Idle
  DR<23>(0x400000u);  // TODO: What does this mean?
  idle_clocks(1); // Run-Test/Idle

  // TODO: Is this neccessary?
  const uint32_t breakpoints[] = {
      0x630000u, 0x670000u, 0x6B0000u, 0x6F0000u,
      0x730000u, 0x770000u, 0x7B0000u, 0x7F0000u,
  };
  for (uint8_t i = 0; i < (sizeof(breakpoints) / sizeof(breakpoints[0])); ++i) {
    DR<23, uint32_t>(breakpoints[i], nullptr);
    idle_clocks(1);
  }

  IR(InstructionSet::Control);
  DR<4>(1);           // TODO: What does this mean?
  idle_clocks(1); // Run-Test/Idle

  IR(InstructionSet::Exit);
}

}
