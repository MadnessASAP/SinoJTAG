#!/usr/bin/env python3
"""Test flash reading with postinit enabled."""

import sys
import time
from tracemalloc import start

from simple_rpc import Interface


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"

    jtag = Interface(port, baudrate=115200)
    print("Init (with postinit)...")
    jtag.init()
    time.sleep(0.1)

    # Verify IDCODE
    jtag.ir(0xE)
    idcode = jtag.dr(0, 16)
    print(f"IDCODE: 0x{idcode:04X}")

    if idcode != 0xC14C:
        print("ERROR: Unexpected IDCODE, target may not be responding")
        return

    print("\n=== Flash Read Test ===\n")

    data = []
    addr = 0x0
    start_time = time.monotonic()
    while addr < 0x10000:
        data += jtag.flash_read(addr)
        addr = len(data)
        run_time = time.monotonic() - start_time
        print(f"Read 0x{len(data):04X} {addr / run_time:.0f} BPS", end="\r")

    with open("./dump.bin", "wb") as file:
        file.write(bytearray(data))

    print(f"\nRead {len(data)} bytes")


if __name__ == "__main__":
    main()
