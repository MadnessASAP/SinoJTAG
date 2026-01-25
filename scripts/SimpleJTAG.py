#!/usr/bin/env python3
"""
SimpleJTAG RPC client - Scan IR with 32-bit DR reads.

Usage: python SimpleJTAG.py [serial_port]
"""

import sys

from simple_rpc import Interface


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"

    print(f"Connecting to {port}...")
    jtag = Interface(port, 115200)

    print("Initializing JTAG interface...")
    jtag.phy_init()
    jtag.tap_init()

    print("\nScanning IR (4-bit) with 32-bit DR reads:\n")
    print("IR   | DR (32-bit)")
    print("-----|------------")

    for ir_val in range(16):
        _ = jtag.tap_ir(ir_val)
        dr_val = jtag.tap_dr(0, 32)
        print(f"0x{ir_val:X}  | 0x{dr_val:08X}")

    print("\nDone.")


if __name__ == "__main__":
    main()
