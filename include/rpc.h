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

namespace rpc {

/** Initialize serial communication. */
void setup();

/** Process incoming RPC requests. */
void loop();

namespace tap {

/** Initialize JTAG interface (includes SinoWealth entry sequence). */
void init();

/** Return the current TAP state (0-15). */
uint8_t state();

/** Force TAP to Test-Logic-Reset. */
void reset();

/** Move TAP to a target state (0-15). */
void goto_state(uint8_t target);

/** Shift instruction register (4-bit). */
uint8_t ir(uint8_t out);

/** Shift data register (up to 32 bits). */
uint32_t dr(uint32_t out, uint8_t bits);

/** Select BYPASS register. */
void bypass();

/** Read IDCODE (16-bit for SinoWealth). */
uint16_t idcode();

/** Emit idle clocks in Run-Test/Idle. */
void idle_clocks(uint8_t count);

}  // namespace tap
}  // namespace rpc
