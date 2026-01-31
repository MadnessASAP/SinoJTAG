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

Much of the SinoWealth specific information is heavily derived from (if not directly includes code from) https://github.com/gashtaan/sinowealth-8051-dumper and https://github.com/gashtaan/sinowealth-8051-bl-updater. Their work has been indispensable.

**Their code is licensed under GPLv3 and as a result this repo and any other derivatives are also.**

[@swiftgeek](https://github.com/swiftgeek) probably also deserves recognition for their work.

---

SinoJTAG is an AVR-native JTAG interface for the Arduino Uno R3 (ATmega328P), focused on SinoWealth 8051 MCUs. It provides a layered design for target-specific debug/ICP workflows including flash read, erase, and programming. The firmware uses avr-libc with direct register access; the Arduino core is only required to support SimpleRPC for the host interface.

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                # Build firmware
pio run -t upload      # Build and upload to Arduino Uno
pio run -t monitor     # Open serial monitor (115200 baud)
```

## Python CLI

The `scripts/sinojtag` package provides a command-line interface for flash operations.

```bash
cd scripts
python -m sinojtag --help

# Read 8KB from flash
python -m sinojtag read -o dump.bin -s 0x2000

# Erase and program from Intel HEX or binary
python -m sinojtag flash firmware.hex -v

# Verify flash contents
python -m sinojtag verify firmware.bin
```

The package can also be used as a library:

```python
from sinojtag import FlashIO

with FlashIO("/dev/ttyACM0") as flash:
    flash.seek(0)
    data = flash.read(4096)
```

## Pin Mapping

| Signal | AVR Pin | Arduino Pin |
|--------|---------|-------------|
| TCK    | PD5     | D5          |
| TMS    | PD3     | D3          |
| TDI    | PD4     | D4          |
| TDO    | PD2     | D2          |
| VREF   | PD6     | D6          |

## Architecture

### Firmware

- **PHY Layer** (`lib/SimpleJTAG/include/SimpleJTAG/phy.h`) — Stateless GPIO bit-banging with direct AVR register manipulation. LSB-first bit streaming with ~500 kHz TCK.
- **TAP Layer** (`lib/SimpleJTAG/include/SimpleJTAG/tap.h`) — Template class tracking JTAG TAP state machine. BFS-based optimal path finding for state transitions. IR/DR shift operations with arbitrary bit widths.
- **SinoWealth Target** (`include/sinowealth/`) — Target-specific entry sequences and ICP operations built on the generic PHY/TAP layers.
- **RPC Interface** (`include/rpc.h`) — SimpleRPC bindings exposing flash read/write/erase to the host.

### Python Package

```
scripts/sinojtag/
├── __init__.py    # Public API (FlashIO, FlashDevice)
├── flash.py       # Device interface classes
├── ihex.py        # Intel HEX format parsing
└── __main__.py    # CLI entry point
```

## Configuration

Pin mappings and timing are configured in `lib/SimpleJTAG/include/SimpleJTAG/config.h`.
