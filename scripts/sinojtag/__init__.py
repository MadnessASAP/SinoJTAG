"""SinoWealth Flash Programmer Interface.

Provides a file-like interface for reading, erasing, and writing
SinoWealth MCU flash memory via Arduino-based JTAG programmer.
"""

from .flash import (
    ERASE_BLOCK_SIZE,
    MAX_TRANSFER_SIZE,
    FlashDevice,
    FlashIO,
)

__all__ = [
    "ERASE_BLOCK_SIZE",
    "MAX_TRANSFER_SIZE",
    "FlashDevice",
    "FlashIO",
]
