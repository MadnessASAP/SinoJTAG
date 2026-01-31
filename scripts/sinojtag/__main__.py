"""SinoWealth Flash Programmer CLI."""

import argparse
import sys
import time
from dataclasses import dataclass
from typing import cast

from . import ihex
from .flash import ERASE_BLOCK_SIZE, MAX_TRANSFER_SIZE, FlashIO


class _ProgressBar:
    """Terminal progress bar with address and data rate display."""

    _operation: str
    _total: int
    _current: int
    _address: int
    _start_time: float
    _bar_width: int

    def __init__(
        self, operation: str, total: int, start_address: int = 0, bar_width: int = 20
    ):
        self._operation = operation
        self._total = total
        self._current = 0
        self._address = start_address
        self._start_time = time.perf_counter()
        self._bar_width = bar_width

    def update(self, bytes_processed: int, current_address: int) -> None:
        """Update progress bar with new values."""
        self._current += bytes_processed
        self._address = current_address
        self._render()

    def _render(self) -> None:
        """Render the progress bar to terminal."""
        elapsed = time.perf_counter() - self._start_time
        rate = self._current / elapsed if elapsed > 0 else 0

        if self._total > 0:
            percent = min(100, (self._current * 100) // self._total)
            filled = (self._current * self._bar_width) // self._total
        else:
            percent = 100
            filled = self._bar_width

        bar = "#" * filled + "-" * (self._bar_width - filled)

        line = f"\r{self._operation}: [{bar}] {percent:3d}% @ 0x{self._address:04X} ({self._format_rate(rate)})"
        _ = sys.stdout.write(line)
        _ = sys.stdout.flush()

    @staticmethod
    def _format_rate(rate: float) -> str:
        """Format data rate with appropriate unit."""
        if rate >= 1024:
            return f"{rate / 1024:.1f} KB/s"
        return f"{rate:.0f} B/s"

    def finish(self) -> None:
        """Complete the progress bar and print summary."""
        elapsed = time.perf_counter() - self._start_time
        rate = self._current / elapsed if elapsed > 0 else 0

        _ = sys.stdout.write("\n")
        print(
            f"{self._operation} complete: {self._current} bytes in {elapsed:.2f}s ({self._format_rate(rate)})"
        )


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
    format: str  # "auto", "ihex", or "binary"


@dataclass
class _VerifyArgs:
    port: str
    baudrate: int
    input: str
    address: int


def _parse_int(value: str) -> int:
    """Parse integer with support for hex (0x) prefix."""
    return int(value, 0)


def _read_with_progress(flash: FlashIO, address: int, size: int, label: str) -> bytes:
    """Read data from flash with progress bar display."""
    progress = _ProgressBar(label, size, address)
    result = bytearray()
    current_addr = address
    remaining = size

    while remaining > 0:
        chunk_size = min(remaining, MAX_TRANSFER_SIZE)
        chunk = flash.read_chunk(current_addr, chunk_size)
        if not chunk:
            break
        result.extend(chunk)
        current_addr += len(chunk)
        remaining -= len(chunk)
        progress.update(len(chunk), current_addr)

    progress.finish()
    return bytes(result)


def _write_with_progress(flash: FlashIO, address: int, data: bytes, label: str) -> int:
    """Write data to flash with progress bar display."""
    progress = _ProgressBar(label, len(data), address)
    current_addr = address
    total_written = 0

    while total_written < len(data):
        chunk = data[total_written : total_written + MAX_TRANSFER_SIZE]
        written = flash.write_chunk(current_addr, chunk)
        if written <= 0:
            break
        total_written += written
        current_addr += written
        progress.update(written, current_addr)

    progress.finish()
    return total_written


def _erase_with_progress(flash: FlashIO, address: int, size: int, label: str) -> int:
    """Erase flash blocks with progress bar display."""
    start_block = (address // ERASE_BLOCK_SIZE) * ERASE_BLOCK_SIZE
    end_addr = address + size
    total_size = end_addr - start_block

    progress = _ProgressBar(label, total_size, start_block)
    blocks_erased = 0

    for addr in range(start_block, end_addr, ERASE_BLOCK_SIZE):
        if flash.erase_block(addr):
            blocks_erased += 1
        progress.update(ERASE_BLOCK_SIZE, addr + ERASE_BLOCK_SIZE)

    progress.finish()
    return blocks_erased


def _cmd_read(args: _ReadArgs) -> int:
    """Read flash contents to file."""
    with FlashIO(args.port, args.baudrate) as flash:
        data = _read_with_progress(flash, args.address, args.size, "Reading")

    with open(args.output, "wb") as f:
        _ = f.write(data)

    print(f"Saved to {args.output}")
    return 0


def _cmd_erase(args: _EraseArgs) -> int:
    """Erase flash blocks."""
    with FlashIO(args.port, args.baudrate) as flash:
        blocks = _erase_with_progress(flash, args.address, args.size, "Erasing")

    print(f"Erased {blocks} block(s)")
    return 0


def _cmd_flash(args: _FlashArgs) -> int:
    """Program flash from file."""
    with open(args.input, "rb") as f:
        raw_data = f.read()

    # Determine format and parse
    use_ihex = args.format == "ihex" or (args.format == "auto" and ihex.detect(raw_data))

    if use_ihex:
        try:
            segments = ihex.parse(raw_data)
            start_addr, data = ihex.merge_segments(segments, args.address)
            print(
                f"Intel HEX: {len(segments)} segment(s), {len(data)} bytes at 0x{start_addr:04X}"
            )
        except ValueError as e:
            print(f"Error parsing Intel HEX: {e}")
            return 1
    else:
        data = raw_data
        start_addr = args.address

    with FlashIO(args.port, args.baudrate) as flash:
        if not args.no_erase:
            blocks = _erase_with_progress(flash, start_addr, len(data), "Erasing")
            print(f"Erased {blocks} block(s)")

        _ = _write_with_progress(flash, start_addr, data, "Writing")

    if args.verify:
        return _verify_data(args.port, args.baudrate, start_addr, data)

    return 0


def _cmd_verify(args: _VerifyArgs) -> int:
    """Verify flash contents against file."""
    with open(args.input, "rb") as f:
        expected = f.read()

    return _verify_data(args.port, args.baudrate, args.address, expected)


def _verify_data(port: str, baudrate: int, address: int, expected: bytes) -> int:
    """Compare flash contents against expected data."""
    with FlashIO(port, baudrate) as flash:
        actual = _read_with_progress(flash, address, len(expected), "Verifying")

    if actual == expected:
        print("Verification PASSED")
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
        prog="sinojtag",
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
    _ = flash_parser.add_argument(
        "-f",
        "--format",
        choices=["auto", "ihex", "binary"],
        default="auto",
        help="Input file format (default: auto-detect)",
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
                    format=cast(str, ns.format),
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
    sys.exit(main())
