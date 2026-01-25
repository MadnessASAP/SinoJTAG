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

# SinoJTAG

---

Much of the SinoWealth specific information is heavily derived from (if not directly include code) from https://github.com/gashtaan/sinowealth-8051-dumper and https://github.com/gashtaan/sinowealth-8051-bl-updater. Their work has been indispensible.

**Their code is licensed under GPLv3 and as a result this repo and any other derivatives are also.**

[@swiftgeek](https://github.com/swiftgeek) probably also deserves recognition for their work.

---

SimpleJTAG is an AVR-native JTAG interface for the Arduino Uno R3 (ATmega328p). It is focused on SinoWealth 8051 MCUs and provides a layered design to support target-specific debug/ICP workflows for flash and RAM access. The implementation uses avr-libc only. The Arduino dependency is required to support SimpleRPC for providing a host interface.

## Structure

- Stateless PHY: Direct GPIO control and bit-level JTAG primitives.
- Stateful TAP: Tracks TAP state and provides IR/DR helpers.
- Target layer: Target-specific operations (to be added).

Key headers:
- `lib/include/SimpleJTAG/phy.h`: Generic stateless interface to bit-bang GPIO.
- `lib/include/SimpleJTAG/tap.h`: Generic TAP state tracker and helpers for reading/writing registers and common JTAG functions.
- `lib/include/SimpleJTAG/config.h`: interface configurations, specific for Arduino UNO R3.
- `include/sinowealth/*.h`: SinoWealth specific variants of generic interfaces.
- `include/rpc.h`: Functions for SimpleRPC host interface.

## Configuration

- Set `Config` in `lib/include/SimpleJTAG/config.h` to actual AVR PORT/DDR/PIN mappings.
