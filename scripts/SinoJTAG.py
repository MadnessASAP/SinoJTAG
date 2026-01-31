#!/usr/bin/env python3
"""SinoWealth Flash Programmer Interface

Provides a file-like interface for reading, erasing, and writing
SinoWealth MCU flash memory via Arduino-based JTAG programmer.
"""

import argparse
from collections.abc import Buffer, Iterator
from dataclasses import dataclass
from io import RawIOBase
from types import TracebackType
from typing import Self, cast, override

from simple_rpc import Interface

# Hardware constraints
MAX_TRANSFER_SIZE = 256  # Arduino RAM limit
ERASE_BLOCK_SIZE = 1024  # Target flash erase block size


@dataclass
class _ReadArgs:
    port: str
    baudrate: int
    output: str
    address: int
    size: int


@dataclass
class _EraseArgs:
    port: str
    baudrate: int
    address: int
    size: int


@dataclass
class _FlashArgs:
    port: str
    baudrate: int
    input: str
    address: int
    no_erase: bool
    verify: bool


@dataclass
class _VerifyArgs:
    port: str
    baudrate: int
    input: str
    address: int


class FlashDevice:
    """Low-level flash access via SimpleRPC."""

    _rpc: Interface
    _initialized: bool

    def __init__(self, port: str = "/dev/ttyACM0", baudrate: int = 115200):
        self._rpc = Interface(port, baudrate)
        self._initialized = False

    def open(self) -> None:
        """Initialize the JTAG interface."""
        if not self._initialized:
            self._rpc.phy_init()
            self._initialized = True

    def close(self) -> None:
        """Release the JTAG interface."""
        if self._initialized:
            _ = self._rpc.phy_reset()
            self._initialized = False

    def __del__(self) -> None:
        self._rpc.phy_stop()

    def read_chunk(self, address: int, size: int) -> bytes:
        """Read up to MAX_TRANSFER_SIZE bytes from flash."""
        size = min(size, MAX_TRANSFER_SIZE)
        data = self._rpc.icp_read(address, size)
        return bytes(data)

    def write_chunk(self, address: int, data: bytes) -> int:
        """Write up to MAX_TRANSFER_SIZE bytes to flash. Returns bytes written."""
        data = data[:MAX_TRANSFER_SIZE]
        return self._rpc.icp_write(address, list(data))

    def erase_block(self, address: int) -> bool:
        """Erase the 1KB block containing the given address."""
        block_address = (address // ERASE_BLOCK_SIZE) * ERASE_BLOCK_SIZE
        return self._rpc.icp_erase(block_address)


class FlashIO(RawIOBase):
    """File-like interface for flash memory.

    Supports reading and writing with automatic chunking and
    block erase handling. Implements Python's RawIOBase interface.

    Example:
        with FlashIO("/dev/ttyACM0") as flash:
            # Read 4KB from address 0
            flash.seek(0)
            data = flash.read(4096)

            # Write data at address 0x1000
            flash.seek(0x1000)
            flash.write(b"Hello, Flash!")

            # Erase a block
            flash.erase(0x2000)
    """

    _device: FlashDevice
    _position: int

    def __init__(self, port: str = "/dev/ttyACM0", baudrate: int = 115200):
        super().__init__()
        self._device = FlashDevice(port, baudrate)
        self._position = 0

    @override
    def __enter__(self) -> Self:
        _ = super().__enter__()
        self._device.open()
        return self

    @override
    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self._device.close()
        super().__exit__(exc_type, exc_val, traceback)

    @override
    def readable(self) -> bool:
        return True

    @override
    def writable(self) -> bool:
        return True

    @override
    def seekable(self) -> bool:
        return True

    @override
    def readinto(self, b: Buffer) -> int:
        """Read bytes into a pre-allocated buffer."""
        buffer = memoryview(b).cast("B")
        size = len(buffer)
        if size == 0:
            return 0

        total_read = 0
        while total_read < size:
            chunk_size = min(size - total_read, MAX_TRANSFER_SIZE)
            chunk = self._device.read_chunk(self._position, chunk_size)
            if not chunk:
                break
            buffer[total_read : total_read + len(chunk)] = chunk
            total_read += len(chunk)
            self._position += len(chunk)

        return total_read

    @override
    def read(self, size: int = -1) -> bytes:
        """Read up to size bytes from flash at current position."""
        if size == 0:
            return b""
        if size < 0:
            raise ValueError("Must specify a positive read size for flash")

        result = bytearray()
        remaining = size

        while remaining > 0:
            chunk_size = min(remaining, MAX_TRANSFER_SIZE)
            chunk = self._device.read_chunk(self._position, chunk_size)
            if not chunk:
                break
            result.extend(chunk)
            self._position += len(chunk)
            remaining -= len(chunk)

        return bytes(result)

    @override
    def write(self, b: Buffer) -> int:
        """Write data to flash at current position.

        Note: The flash block must be erased before writing.
        Use erase() to erase blocks as needed.
        """
        data = bytes(memoryview(b))
        if not data:
            return 0

        total_written = 0

        while total_written < len(data):
            chunk = data[total_written : total_written + MAX_TRANSFER_SIZE]
            written = self._device.write_chunk(self._position, chunk)
            if written <= 0:
                break
            total_written += written
            self._position += written

        return total_written

    @override
    def seek(self, offset: int, whence: int = 0) -> int:
        """Move to a new position in flash.

        Args:
            offset: Byte offset
            whence: 0=absolute, 1=relative to current, 2=not supported

        Returns:
            New absolute position
        """
        if whence == 0:
            self._position = offset
        elif whence == 1:
            self._position += offset
        elif whence == 2:
            raise OSError("Cannot seek from end on flash device")
        else:
            raise ValueError(f"Invalid whence value: {whence}")

        if self._position < 0:
            self._position = 0

        return self._position

    @override
    def tell(self) -> int:
        """Return current position."""
        return self._position

    def erase(self, address: int | None = None) -> bool:
        """Erase the 1KB block at the given address (or current position).

        Args:
            address: Block address to erase, or None to use current position

        Returns:
            True if erase succeeded
        """
        if address is None:
            address = self._position
        return self._device.erase_block(address)

    def erase_range(self, start: int, size: int) -> int:
        """Erase all blocks covering the given address range.

        Args:
            start: Starting address
            size: Number of bytes to cover

        Returns:
            Number of blocks erased
        """
        start_block = (start // ERASE_BLOCK_SIZE) * ERASE_BLOCK_SIZE
        end_address = start + size
        blocks_erased = 0

        for addr in range(start_block, end_address, ERASE_BLOCK_SIZE):
            if self._device.erase_block(addr):
                blocks_erased += 1

        return blocks_erased

    def program(self, address: int, data: Buffer, erase: bool = True) -> int:
        """Convenience method to erase and write data.

        Args:
            address: Starting address
            data: Data to write
            erase: If True, erase required blocks first

        Returns:
            Number of bytes written
        """
        data_bytes = bytes(memoryview(data))
        if erase:
            _ = self.erase_range(address, len(data_bytes))

        _ = self.seek(address)
        return self.write(data_bytes)


class FlashReader(Iterator[int]):
    """Iterator that yields bytes from flash one at a time.

    Useful for streaming reads without loading entire content into memory.
    """

    _flash: FlashIO
    _buffer: bytearray
    _remaining: int

    def __init__(self, flash: FlashIO, address: int, size: int):
        self._flash = flash
        self._buffer = bytearray()
        self._remaining = size
        _ = flash.seek(address)

    @override
    def __iter__(self) -> "FlashReader":
        return self

    @override
    def __next__(self) -> int:
        if len(self._buffer) == 0:
            if self._remaining > 0:
                chunk_size = min(self._remaining, MAX_TRANSFER_SIZE)
                chunk = self._flash.read(chunk_size)
                if chunk:
                    self._buffer.extend(chunk)
                    self._remaining -= len(chunk)

        if len(self._buffer) == 0:
            raise StopIteration

        return self._buffer.pop(0)


# Backward compatibility aliases
Flash = FlashIO
ReadIterator = FlashReader


def _parse_int(value: str) -> int:
    """Parse integer with support for hex (0x) prefix."""
    return int(value, 0)


def _cmd_read(args: _ReadArgs) -> int:
    """Read flash contents to file."""
    print(f"Reading {args.size} bytes from 0x{args.address:04X}...")

    with FlashIO(args.port, args.baudrate) as flash:
        _ = flash.seek(args.address)
        data = flash.read(args.size)

    with open(args.output, "wb") as f:
        _ = f.write(data)

    print(f"Read {len(data)} bytes to {args.output}")
    return 0


def _cmd_erase(args: _EraseArgs) -> int:
    """Erase flash blocks."""
    start_block = (args.address // ERASE_BLOCK_SIZE) * ERASE_BLOCK_SIZE
    end_addr = args.address + args.size
    num_blocks = (end_addr - start_block + ERASE_BLOCK_SIZE - 1) // ERASE_BLOCK_SIZE

    print(f"Erasing {num_blocks} block(s) from 0x{start_block:04X}...")

    with FlashIO(args.port, args.baudrate) as flash:
        erased = flash.erase_range(args.address, args.size)

    print(f"Erased {erased} block(s)")
    return 0


def _cmd_flash(args: _FlashArgs) -> int:
    """Program flash from file."""
    with open(args.input, "rb") as f:
        data = f.read()

    print(f"Programming {len(data)} bytes at 0x{args.address:04X}...")

    with FlashIO(args.port, args.baudrate) as flash:
        if not args.no_erase:
            blocks = flash.erase_range(args.address, len(data))
            print(f"Erased {blocks} block(s)")

        _ = flash.seek(args.address)
        written = flash.write(data)

    print(f"Wrote {written} bytes")

    if args.verify:
        print("Verifying...")
        return _verify_data(args.port, args.baudrate, args.address, data)

    return 0


def _cmd_verify(args: _VerifyArgs) -> int:
    """Verify flash contents against file."""
    with open(args.input, "rb") as f:
        expected = f.read()

    print(f"Verifying {len(expected)} bytes at 0x{args.address:04X}...")
    return _verify_data(args.port, args.baudrate, args.address, expected)


def _verify_data(port: str, baudrate: int, address: int, expected: bytes) -> int:
    """Compare flash contents against expected data."""
    with FlashIO(port, baudrate) as flash:
        _ = flash.seek(address)
        actual = flash.read(len(expected))

    if actual == expected:
        print("Verification passed")
        return 0

    # Find first mismatch
    for i, (a, e) in enumerate(zip(actual, expected, strict=False)):
        if a != e:
            print(
                f"Verification FAILED at 0x{address + i:04X}: expected 0x{e:02X}, got 0x{a:02X}"
            )
            return 1

    if len(actual) != len(expected):
        print(
            f"Verification FAILED: size mismatch (expected {len(expected)}, got {len(actual)})"
        )
        return 1

    return 1  # Should not reach here


def main() -> int:
    """Main entry point for CLI."""
    parser = argparse.ArgumentParser(
        prog="SinoJTAG",
        description="SinoWealth MCU Flash Programmer",
    )
    _ = parser.add_argument(
        "-p",
        "--port",
        default="/dev/ttyACM0",
        help="Serial port (default: /dev/ttyACM0)",
    )
    _ = parser.add_argument(
        "-b",
        "--baudrate",
        type=int,
        default=115200,
        help="Baud rate (default: 115200)",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    # Read command
    read_parser = subparsers.add_parser("read", help="Read flash to file")
    _ = read_parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Output file path",
    )
    _ = read_parser.add_argument(
        "-a",
        "--address",
        type=_parse_int,
        default=0,
        help="Start address (default: 0)",
    )
    _ = read_parser.add_argument(
        "-s",
        "--size",
        type=_parse_int,
        required=True,
        help="Number of bytes to read",
    )

    # Erase command
    erase_parser = subparsers.add_parser("erase", help="Erase flash blocks")
    _ = erase_parser.add_argument(
        "-a",
        "--address",
        type=_parse_int,
        default=0,
        help="Start address (default: 0)",
    )
    _ = erase_parser.add_argument(
        "-s",
        "--size",
        type=_parse_int,
        required=True,
        help="Number of bytes to erase (rounds up to block boundary)",
    )

    # Flash/write command
    flash_parser = subparsers.add_parser(
        "flash",
        aliases=["write"],
        help="Program flash from file",
    )
    _ = flash_parser.add_argument(
        "input",
        help="Input file path",
    )
    _ = flash_parser.add_argument(
        "-a",
        "--address",
        type=_parse_int,
        default=0,
        help="Start address (default: 0)",
    )
    _ = flash_parser.add_argument(
        "--no-erase",
        action="store_true",
        help="Skip erasing before write",
    )
    _ = flash_parser.add_argument(
        "-v",
        "--verify",
        action="store_true",
        help="Verify after programming",
    )

    # Verify command
    verify_parser = subparsers.add_parser("verify", help="Verify flash against file")
    _ = verify_parser.add_argument(
        "input",
        help="File to verify against",
    )
    _ = verify_parser.add_argument(
        "-a",
        "--address",
        type=_parse_int,
        default=0,
        help="Start address (default: 0)",
    )

    ns = parser.parse_args()
    command = cast(str, ns.command)

    match command:
        case "read":
            return _cmd_read(
                _ReadArgs(
                    port=cast(str, ns.port),
                    baudrate=cast(int, ns.baudrate),
                    output=cast(str, ns.output),
                    address=cast(int, ns.address),
                    size=cast(int, ns.size),
                )
            )
        case "erase":
            return _cmd_erase(
                _EraseArgs(
                    port=cast(str, ns.port),
                    baudrate=cast(int, ns.baudrate),
                    address=cast(int, ns.address),
                    size=cast(int, ns.size),
                )
            )
        case "flash" | "write":
            return _cmd_flash(
                _FlashArgs(
                    port=cast(str, ns.port),
                    baudrate=cast(int, ns.baudrate),
                    input=cast(str, ns.input),
                    address=cast(int, ns.address),
                    no_erase=cast(bool, ns.no_erase),
                    verify=cast(bool, ns.verify),
                )
            )
        case "verify":
            return _cmd_verify(
                _VerifyArgs(
                    port=cast(str, ns.port),
                    baudrate=cast(int, ns.baudrate),
                    input=cast(str, ns.input),
                    address=cast(int, ns.address),
                )
            )
        case _:
            parser.print_help()
            return 1


if __name__ == "__main__":
    import sys

    sys.exit(main())
