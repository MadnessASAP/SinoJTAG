"""Flash memory device interface.

Provides low-level and file-like interfaces for SinoWealth MCU flash
memory access via Arduino-based JTAG programmer.
"""

from collections.abc import Buffer
from io import RawIOBase
from types import TracebackType
from typing import Self, override

from simple_rpc import Interface

# Hardware constraints
MAX_TRANSFER_SIZE = 64  # Arduino RAM limit
ERASE_BLOCK_SIZE = 1024  # Target flash erase block size


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

    def read_chunk(self, address: int, size: int) -> bytes:
        """Read a single chunk from flash (up to MAX_TRANSFER_SIZE bytes)."""
        return self._device.read_chunk(address, size)

    def write_chunk(self, address: int, data: bytes) -> int:
        """Write a single chunk to flash (up to MAX_TRANSFER_SIZE bytes)."""
        return self._device.write_chunk(address, data)

    def erase_block(self, address: int) -> bool:
        """Erase a single 1KB block at the given address."""
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
