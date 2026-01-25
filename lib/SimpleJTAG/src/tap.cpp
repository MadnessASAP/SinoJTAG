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

#include "SimpleJTAG/tap.h"

namespace SimpleJTAG {

Tap::Tap() : state_(State::TestLogicReset) {
  static_assert(config::IR_BITS > 0 && config::IR_BITS <= 32, "IR_BITS must be 1..32");
}

/** Configure GPIO for JTAG using the underlying PHY. */
void Tap::init() { }

/** Return the currently tracked TAP state. */
Tap::State Tap::state() const { return state_; }

/** Force TAP to Test-Logic-Reset by holding TMS high for 5 clocks. */
void Tap::reset() {
  for (uint8_t i = 0; i < 5; ++i) {
    Phy::next_state(true);
  }
  state_ = State::TestLogicReset;
}

/** Select IR=IDCODE then read 32 bits from DR. */
uint32_t Tap::idcode() {
  uint32_t idcode;
  IR(InstructionSet::IDCODE);
  DR<32>(0UL, &idcode);
  return idcode;
}

/** Select BYPASS by shifting all-ones into IR. */
void Tap::bypass() {
  IR(InstructionSet::BYPASS);
}

/** Move the TAP to a target state using the shortest TMS sequence. */
void Tap::goto_state(State target) {
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

/** Emit additional idle while keeping TMS low.
 * @warning Only stable in:
 *  @li Run-Test/Idle
 *  @li Shift-DR
 *  @li Shift-IR
 *  @li Pause-DR
 *  @li Pause-IR
 */
void Tap::idle_clocks(uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    step(false);
  }
}

} // SimpleJTAG
