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


namespace SimpleJTAG {

/** TAP controller with state tracking and IR/DR helpers. */
class Tap {
 public:
  /** JTAG TAP state enumeration. */
  enum class State : uint8_t {
    TestLogicReset = 0,
    RunTestIdle = 1,
    SelectDRScan = 2,
    CaptureDR = 3,
    ShiftDR = 4,
    Exit1DR = 5,
    PauseDR = 6,
    Exit2DR = 7,
    UpdateDR = 8,
    SelectIRScan = 9,
    CaptureIR = 10,
    ShiftIR = 11,
    Exit1IR = 12,
    PauseIR = 13,
    Exit2IR = 14,
    UpdateIR = 15,
  };

  Tap();

  /** Return the next TAP state given the current state and TMS */
  static constexpr State next_state(State s, bool tms);

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

  struct InstructionSet {
    static constexpr uint32_t IDCODE = 0x0000000E;
    static constexpr uint32_t BYPASS = 0xFFFFFFFF;
  };

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
  static_assert(bits > 0, "DR bits must be >= 1");

  goto_state(State::ShiftDR);
  Phy::stream_bits<bits, true>(out, in);

  state_ = State::Exit1DR;
  step(true); // State::UpdateDR
}

template <typename T>
void Tap::IR(T out, T* in) {
  goto_state(State::ShiftIR);
  Phy::stream_bits<config::IR_BITS, true>(out, in);

  state_ = State::Exit1IR;
  step(true); // State::UpdateIR
}


constexpr Tap::State Tap::next_state(State s, bool tms) {
  switch (s) {
    case State::TestLogicReset:
      return tms ? State::TestLogicReset : State::RunTestIdle;
    case State::RunTestIdle:
      return tms ? State::SelectDRScan : State::RunTestIdle;
    case State::SelectDRScan:
      return tms ? State::SelectIRScan : State::CaptureDR;
    case State::CaptureDR:
      return tms ? State::Exit1DR : State::ShiftDR;
    case State::ShiftDR:
      return tms ? State::Exit1DR : State::ShiftDR;
    case State::Exit1DR:
      return tms ? State::UpdateDR : State::PauseDR;
    case State::PauseDR:
      return tms ? State::Exit2DR : State::PauseDR;
    case State::Exit2DR:
      return tms ? State::UpdateDR : State::ShiftDR;
    case State::UpdateDR:
      return tms ? State::SelectDRScan : State::RunTestIdle;
    case State::SelectIRScan:
      return tms ? State::TestLogicReset : State::CaptureIR;
    case State::CaptureIR:
      return tms ? State::Exit1IR : State::ShiftIR;
    case State::ShiftIR:
      return tms ? State::Exit1IR : State::ShiftIR;
    case State::Exit1IR:
      return tms ? State::UpdateIR : State::PauseIR;
    case State::PauseIR:
      return tms ? State::Exit2IR : State::PauseIR;
    case State::Exit2IR:
      return tms ? State::UpdateIR : State::ShiftIR;
    case State::UpdateIR:
      return tms ? State::SelectDRScan : State::RunTestIdle;
    default:
      return State::TestLogicReset;
  }
}

}  // namespace SimpleJTAG
