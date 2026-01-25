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

#include <SimpleJTAG/phy.h>

namespace sinowealth {

class Phy : public SimpleJTAG::Phy {
 public:
  enum class Mode : uint8_t {
    READY = 0x00,
    JTAG = 0xA5,
    ICP = 0x69,
    NOT_INITIALIZED = 0xFF
  };

  /**Initialize the JTAG interface for SinoWealth 8051 MCUs
   * Emits special waveforms out JTAG pins that enables JTAG on the target MCU
   */
  void init(bool wait_vref = true);

  /**Resets state to NOT_INITIALIZED and return GPIOs to High Z */
  void stop();

  /**Switch to new SinoWealth mode */
  Mode mode(Mode mode);

  /**Get current mode */
  Mode mode() { return _mode; };

  /**Reset PHY to READY state */
  Mode reset();


 private:
  Mode _mode = Mode::NOT_INITIALIZED;
};

} // namespace sinowealth
