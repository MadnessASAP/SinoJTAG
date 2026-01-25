"""Type stubs for simple_rpc when used with SimpleJTAG firmware."""

from typing import Sequence

class Interface:
    """SimpleRPC interface for SimpleJTAG Arduino firmware.

    Provides RPC methods for JTAG and ICP operations on SinoWealth MCUs.
    """

    def __init__(self, port: str, baudrate: int = 115200) -> None:
        """Connect to SimpleJTAG device.

        Args:
            port: Serial port path (e.g., "/dev/ttyACM0").
            baudrate: Baud rate (default 115200).
        """
        ...

    # PHY layer
    def phy_init(self) -> None:
        """Initialize SinoWealth diagnostics mode."""
        ...

    # TAP layer
    def tap_init(self) -> None:
        """Initialize JTAG interface."""
        ...

    def tap_state(self) -> int:
        """Get current TAP state.

        Returns:
            State value (0-15).
        """
        ...

    def tap_reset(self) -> None:
        """Force TAP to Test-Logic-Reset."""
        ...

    def tap_goto_state(self, target: int) -> None:
        """Move to target TAP state.

        Args:
            target: State value (0-15).
        """
        ...

    def tap_ir(self, out: int) -> int:
        """Shift instruction register.

        Args:
            out: Value to shift out.

        Returns:
            Captured value shifted in.
        """
        ...

    def tap_dr(self, out: int, bits: int) -> int:
        """Shift data register.

        Args:
            out: Value to shift out.
            bits: Bit width (4, 8, 16, 23, or 32).

        Returns:
            Captured value shifted in.
        """
        ...

    def tap_bypass(self) -> None:
        """Select BYPASS register."""
        ...

    def tap_idcode(self) -> int:
        """Read IDCODE.

        Returns:
            Device ID code.
        """
        ...

    def tap_idle_clocks(self, count: int) -> None:
        """Emit idle clocks in Run-Test/Idle state.

        Args:
            count: Number of clock cycles.
        """
        ...

    # ICP layer
    def icp_init(self) -> None:
        """Initialize ICP interface."""
        ...

    def icp_verify(self) -> bool:
        """Perform readback test on ICP.

        Returns:
            True if verification passed.
        """
        ...

    def icp_read(self, address: int, size: int) -> Sequence[int]:
        """Read flash memory via ICP.

        Args:
            address: 16-bit flash address.
            size: Number of bytes to read (8-bit).

        Returns:
            Sequence of bytes read from flash.
        """
        ...
