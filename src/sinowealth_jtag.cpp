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

#include "sinowealth_jtag.h"

#include <util/delay.h>

namespace jtag {
namespace {

/** Send the SinoWealth mode byte MSB-first with extra trailing clocks. */
void send_mode_byte(const IPHYIface& iface, uint8_t mode) {
  iphy_stream_bits(iface, mode, 8, false, nullptr);
  iface.next_state(false);
  iface.next_state(false);
}

}  // namespace

void sinowealth_jtag_init(Tap<4>& tap, const IPHYIface& iface) {
  static constexpr uint8_t kModeJtag = 0xA5u;
  static constexpr uint8_t kIrControl = 2u;
  static constexpr uint8_t kIrData = 3u;
  static constexpr uint8_t kIrExit = 12u;

  send_mode_byte(iface, kModeJtag);
  tap.reset();
  tap.goto_state(Tap<4>::State::RunTestIdle);
  tap.idle_clocks(2);

  tap.IR<uint8_t>(kIrControl, nullptr);
  tap.DR<4, uint8_t>(4, nullptr);
  tap.idle_clocks(1);

  tap.IR<uint8_t>(kIrData, nullptr);
  tap.DR<23, uint32_t>(0x403000u, nullptr);
  tap.idle_clocks(1);
  _delay_us(50);
  tap.DR<23, uint32_t>(0x402000u, nullptr);
  tap.idle_clocks(1);
  tap.DR<23, uint32_t>(0x400000u, nullptr);
  tap.idle_clocks(1);

  const uint32_t breakpoints[] = {
      0x630000u, 0x670000u, 0x6B0000u, 0x6F0000u,
      0x730000u, 0x770000u, 0x7B0000u, 0x7F0000u,
  };
  for (uint8_t i = 0; i < (sizeof(breakpoints) / sizeof(breakpoints[0])); ++i) {
    tap.DR<23, uint32_t>(breakpoints[i], nullptr);
    tap.idle_clocks(1);
  }

  tap.IR<uint8_t>(kIrControl, nullptr);
  tap.DR<4, uint8_t>(1, nullptr);
  tap.idle_clocks(1);

  tap.IR<uint8_t>(kIrExit, nullptr);
}

}  // namespace jtag
