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

#include "iphy.h"

namespace jtag {

/** TAP controller with state tracking and IR/DR helpers. */
template <int IR_BITS>
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

  /** Construct a TAP controller using a non-templated PHY interface. */
  explicit Tap(const IPHYIface& phy) : phy_(phy), state_(State::TestLogicReset) {
    static_assert(IR_BITS > 0 && IR_BITS <= 32, "IR_BITS must be 1..32");
  }

  /** Configure GPIO for JTAG using the underlying PHY. */
  void init() { phy_.init(); }

  /** Run optional pre-init waveform with GPIOs still in reset state. */
  void preinit() {
    if (phy_.preinit) {
      phy_.preinit();
    }
  }

  /** Return the currently tracked TAP state. */
  State state() const { return state_; }

  /** Force TAP to Test-Logic-Reset by holding TMS high for 5 clocks. */
  void reset() {
    for (uint8_t i = 0; i < 5; ++i) {
      phy_.next_state(true);
    }
    state_ = State::TestLogicReset;
  }

  /** Move the TAP to a target state using the shortest TMS sequence. */
  void goto_state(State target) {
    if (state_ == target) {
      return;
    }

    const uint8_t start = static_cast<uint8_t>(state_);
    const uint8_t goal = static_cast<uint8_t>(target);

    uint8_t queue[16];
    uint8_t prev[16];
    uint8_t prev_tms[16];
    bool visited[16] = {false};

    uint8_t head = 0;
    uint8_t tail = 0;
    visited[start] = true;
    queue[tail++] = start;

    while (head < tail && !visited[goal]) {
      const uint8_t s = queue[head++];
      for (uint8_t tms = 0; tms < 2; ++tms) {
        const uint8_t ns =
            static_cast<uint8_t>(next_state(static_cast<State>(s), tms != 0));
        if (!visited[ns]) {
          visited[ns] = true;
          prev[ns] = s;
          prev_tms[ns] = tms;
          queue[tail++] = ns;
        }
      }
    }

    if (!visited[goal]) {
      return;
    }

    uint8_t seq[16];
    uint8_t len = 0;
    for (uint8_t cur = goal; cur != start; cur = prev[cur]) {
      seq[len++] = prev_tms[cur];
    }

    while (len > 0) {
      const bool tms = (seq[--len] != 0);
      step(tms);
    }
  }

  /** Shift an instruction register value and optionally capture output. */
  template <typename T>
  void IR(T out, T* in = nullptr) {
    static_assert(sizeof(T) <= 4, "IR type must be <= 32 bits");
    goto_state(State::ShiftIR);
    const uint32_t capture =
        phy_.stream_bits(static_cast<uint32_t>(out), IR_BITS, true, nullptr);
    if (in) {
      *in = static_cast<T>(capture);
    }
    state_ = State::Exit1IR;
    step(true);
    step(false);
  }

  /** Shift a data register value of a fixed bit width. */
  template <int bits, typename T>
  void DR(T out, T* in = nullptr) {
    static_assert(bits > 0 && bits <= 32, "DR bits must be 1..32");
    static_assert(sizeof(T) <= 4, "DR type must be <= 32 bits");
    goto_state(State::ShiftDR);
    const uint32_t capture =
        phy_.stream_bits(static_cast<uint32_t>(out), bits, true, nullptr);
    if (in) {
      *in = static_cast<T>(capture);
    }
    state_ = State::Exit1DR;
    step(true);
    step(false);
  }

  /** Select BYPASS by shifting all-ones into IR. */
  void bypass() {
    const uint32_t mask =
        (IR_BITS >= 32) ? 0xFFFFFFFFu : static_cast<uint32_t>((1ULL << IR_BITS) - 1ULL);
    IR<uint32_t>(mask, nullptr);
  }

  /** Select IDCODE then read N bits from DR. */
  template <int N = 32, typename T>
  void idcode(T* in) {
    static_assert(N > 0 && N <= 32, "IDCODE bits must be 1..32");
    IR<uint32_t>(kIdcodeInstr, nullptr);
    DR<N, T>(0, in);
  }

  /** Emit additional idle clocks while staying in Run-Test/Idle. */
  void idle_clocks(uint8_t count = 1) {
    for (uint8_t i = 0; i < count; ++i) {
      step(false);
    }
  }

 private:
  /** Default IDCODE instruction value (LSB-first). */
  static constexpr uint32_t kIdcodeInstr = 0xEu;

  /** Apply a single TMS transition and update tracked state. */
  void step(bool tms) {
    phy_.next_state(tms);
    state_ = next_state(state_, tms);
  }

  /** Compute the next TAP state for a given state and TMS value. */
  static inline State next_state(State s, bool tms) {
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

  /** Underlying PHY interface. */
  const IPHYIface& phy_;
  /** Current tracked TAP state. */
  State state_;
};

}  // namespace jtag
