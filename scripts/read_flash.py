#!/usr/bin/env python3

import sys
import time
from typing import Any

from simple_rpc import Interface  # pyright: ignore[reportMissingTypeStubs]


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"

    jtag: Any = Interface(device=port, baudrate=115200)  # pyright: ignore[reportCallIssue]

    print("Init (with postinit)...")
    jtag.phy_init()
    jtag.icp_init()

    if jtag.icp_verify():
        print("ICP init okay")
    else:
        print("ICP init fail")
        return

    print("\n=== Flash Read Test ===\n")

    data = []
    addr = 0x0
    start_time = time.monotonic()
    while addr < 0x10000:
        data += jtag.icp_read(addr, 64)
        addr = len(data)
        run_time = time.monotonic() - start_time
        print(f"Read 0x{len(data):04X} {addr / run_time:.0f} BPS", end="\r")

    with open("./dump.bin", "wb") as file:
        file.write(bytearray(data))

    print(f"\nRead {len(data)} bytes")


if __name__ == "__main__":
    main()
