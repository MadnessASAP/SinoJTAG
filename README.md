<!--
Copyright (C) 2026 Michael "ASAP" Weinrich

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-->

# SimpleJTAG

SimpleJTAG is an AVR-native JTAG interface for the Arduino Uno R3
(ATmega328p). It is focused on SinoWealth 8051 MCUs and provides a
layered design to support target-specific debug/ICP workflows for
flash and RAM access. The implementation uses avr-libc only and
bit-bangs JTAG over GPIO without the Arduino framework.

## Structure

- Stateless PHY: Direct GPIO control and bit-level JTAG primitives.
- Stateful TAP: Tracks TAP state and provides IR/DR helpers.
- Target layer: Target-specific operations (to be added).

Key headers:
- `include/phy.h`: `Phy<Pins, Timing>` for fast GPIO JTAG.
- `include/iphy.h`: `IPHYIface` function table used by TAP.
- `include/tap.h`: `Tap<IR_BITS>` state machine and IR/DR helpers.
- `include/config.h`: pin mapping placeholder.
- `include/sinowealth.h`: SinoWealth diagnostic/JTAG entry helpers.

## Program flow

1) `main` constructs a `Tap<IR_BITS>` using an `IPHYIface` created by
   `IPHY<Pins, Timing>::iface()`.

2) Target-specific entry can run before GPIO init (e.g. SinoWealth
   `sinowealth::diag_enter()`), while pins are still at power-on defaults.

3) `init()` configures JTAG GPIO direction and idle levels:
   - TCK/TMS/TDI outputs, TDO input
   - TCK low, TMS high, TDI low

4) Target-specific JTAG entry can run after init (e.g. `sinowealth::jtag_enter()`
   followed by `sinowealth::postinit()`).

5) TAP operations:
   - `reset()` drives TMS high for 5+ clocks to enter Test-Logic-Reset.
   - `goto_state()` emits the shortest TMS sequence to a target state.
   - `IR()` and `DR()` shift bits LSB-first via `IPHYIface::stream_bits`.
   - `bypass()` and `idcode()` are thin helpers built on IR/DR.

6) PHY operations:
   - `next_state()` drives TMS and pulses TCK.
   - `stream_bits()` shifts bits and optionally captures TDO.

## Configuration

- Set `Config` in `include/config.h` to actual AVR PORT/DDR/PIN mappings.
- Adjust timing by providing a custom `Timing` type if a slower TCK is needed.
- Define `JTAG_IR_BITS` and `JTAG_BRINGUP` in your build for bring-up testing.
