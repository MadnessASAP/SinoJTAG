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

#include <stddef.h>
#include <stdint.h>

#include "phy.h"
#include "tap_state.h"
#ifndef IR_BITS
#define IR_BITS 4
#endif


namespace jtag {

/** TAP controller with state tracking and IR/DR helpers. */
class Tap {
 public:

  Tap();

  /** Configure GPIO for JTAG using the underlying PHY. */
  void init();

  /** Return the currently tracked TAP state. */
  State state() const;

  /** Force TAP to Test-Logic-Reset by holding TMS high for 5 clocks. */
  void reset();

  /** Move the TAP to a target state using the shortest TMS sequence. */
  void goto_state(State target);

  /** Select BYPASS by shifting all-ones into IR.
   * @post state = Pause-IR
   */
  void bypass();

  /** Select IR=IDCODE then read 32 bits from DR.
   * @post state = Pause-DR
   */
  uint32_t idcode();

  /** Shift an instruction register value and optionally capture output.
   * @post state = Update-IR
   */
  template <typename T>
  void IR(T out, T* in = nullptr);

  /** Shift a data register value of a fixed bit width.
   * @post state = Update-DR
   */
  template <int bits, typename T>
  void DR(T out, T* in = nullptr);

  /** Emit additional idle while keeping TMS low.
   * @warning Only stable in:
   *  @li Run-Test/Idle
   *  @li Shift-DR
   *  @li Shift-IR
   *  @li Pause-DR
   *  @li Pause-IR
   */
  void idle_clocks(uint8_t count = 1);

  static constexpr struct InstructionSet {
    uint32_t IDCODE;
    uint32_t BYPASS;
  } Instruction{0x0000000E, 0xFFFFFFFF};
 private:

  /** Apply a single TMS transition and update tracked state. */
  void step(bool tms) {
    Phy::next_state(tms);
    state_ = next_state(state_, tms);
  }

  /** Current tracked TAP state. */
  State state_;
}; // class Tap

template <int bits, typename T>
void Tap::DR(T out, T* in) {
  static_assert(bits > 0 && bits <= 32, "DR bits must be 1..32");
  static_assert(sizeof(T) <= 4, "DR type must be <= 32 bits");

  goto_state(State::ShiftDR);
  Phy::stream_bits<bits, true>(out, in);

  state_ = State::Exit1DR;
  step(true); // State::UpdateDR
}

template <typename T>
void Tap::IR(T out, T* in) {
  static_assert(sizeof(T) <= 4, "IR type must be <= 32 bits");

  goto_state(State::ShiftIR);
  Phy::stream_bits<IR_BITS, true>(out, in);

  state_ = State::Exit1IR;
  step(true); // State::UpdateIR
}

}  // namespace jtag
