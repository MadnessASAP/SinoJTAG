from collections.abc import Iterable, Sequence
from typing import override

from simple_rpc import Interface


class ReadIterator(Iterable[int]):
    def __init__(self, flash: Flash, address: int, size: int):
        self._buffer: bytearray = bytearray()
        self._flash: Flash = flash
        self._next: int = address
        self._end: int = address + size

    @override
    def __iter__(self) -> ReadIterator:
        return self

    def __next__(self) -> int:
        if len(self._buffer) == 0:
            if self._next < self._end:
                size = min(self._end - self._next, 128)
                self._buffer.extend(self._flash.read(self._next, size))
                self._next += size
            else:
                raise StopIteration

        return self._buffer.pop(0)


class Flash:
    """Access to MCU flash via SimpleRPC"""

    _rpc: Interface

    def __init__(self, port: str = "/dev/ttyACM0"):
        self._rpc = Interface(port, 115200)

    def __enter__(self) -> Flash:
        self._rpc.phy_init()
        return self

    def __exit__(self, exc_type, exc_val, traceback):
        _ = self._rpc.phy_reset()

    def __del__(self):
        self._rpc.phy_stop()

    def read(self, address: int, size: int) -> Sequence[int]:
        buffer: list[int] = []
        end = address + size
        # Read at most 128 bytes at a time
        for addr in range(address, end, 128):
            if addr + 128 > end:
                size = end - addr
            else:
                size = 128
            buffer += self._rpc.icp_read(addr, size)
        return buffer

    def write(self, address: int, data: Sequence[int]) -> int:
        return self._rpc.icp_write(address, data)

    def erase(self, address: int) -> bool:
        return self._rpc.icp_erase(address)
