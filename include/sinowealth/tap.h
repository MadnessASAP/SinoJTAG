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

#include <SimpleJTAG/tap.h>

namespace sinowealth {

class Tap : public SimpleJTAG::Tap {
 public:
  /** Init TAP state and setup target SinoWealth MCU for proper JTAG usage */
  void init();

  /** Leaves JTAG and returns to SinoWealth READY mode
    * Equivalent to sinowealth::Phy.reset()
    */
  void exit();

  struct InstructionSet : SimpleJTAG::Tap::InstructionSet {
    static constexpr uint8_t Control = 0x2;
    static constexpr uint8_t Data = 0x3;
    static constexpr uint8_t Exit = 0xC;
  };
};

 }
