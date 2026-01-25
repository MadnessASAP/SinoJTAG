#!/usr/bin/env python3

import sys
from collections.abc import Sequence

from SinoJTAG import Flash, ReadIterator


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
    output = sys.argv[2] if len(sys.argv) > 2 else "dump.bin"

    buffer: Sequence[int] = []

    with Flash(port) as flash:
        for byte in ReadIterator(flash, 0x0000, 0x10000):
            buffer.append(byte)
            if len(buffer) % 1024 == 0:
                print(f"\rRead 0x{len(buffer):04X} bytes", end="")

    print(f"\nRead {len(buffer)} bytes")

    with open(output, "wb") as file:
        _ = file.write(bytearray(buffer))


if __name__ == "__main__":
    main()
