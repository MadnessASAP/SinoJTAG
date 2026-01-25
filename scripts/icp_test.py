#!/usr/bin/env python3

import sys

from simple_rpc import Interface


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"

    jtag = Interface(port, baudrate=115200)
    print("RPC Started")
    jtag.phy_init()
    print("PHY initialized")
    jtag.icp_init()
    print("ICP initialized")
    result = jtag.icp_verify()
    print(f"Verify? {result}")

    buffer = jtag.icp_read(0x0000, 128)
    print(f"Read: {len(buffer)} bytes")
    for b in buffer:
        print(f"{b:02X} ", end="")


if __name__ == "__main__":
    main()
