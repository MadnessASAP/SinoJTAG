"""Intel HEX file format parsing."""

from dataclasses import dataclass


@dataclass
class IHexSegment:
    """A contiguous segment of data from an Intel HEX file."""

    address: int
    data: bytes


def parse(content: bytes) -> list[IHexSegment]:
    """Parse Intel HEX format and return list of data segments.

    Supports record types:
      00 - Data
      01 - End of File
      02 - Extended Segment Address
      04 - Extended Linear Address
    """
    segments: list[IHexSegment] = []
    current_data = bytearray()
    current_base = 0
    segment_start = 0
    extended_addr = 0

    try:
        text = content.decode("ascii")
    except UnicodeDecodeError as e:
        raise ValueError("Invalid Intel HEX: not ASCII text") from e

    for line_num, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise ValueError(f"Invalid Intel HEX at line {line_num}: missing ':'")

        try:
            record = bytes.fromhex(line[1:])
        except ValueError as e:
            raise ValueError(f"Invalid Intel HEX at line {line_num}: bad hex") from e

        if len(record) < 5:
            raise ValueError(f"Invalid Intel HEX at line {line_num}: too short")

        byte_count = record[0]
        addr = (record[1] << 8) | record[2]
        record_type = record[3]
        data = record[4:-1]
        checksum = record[-1]

        # Verify checksum
        calculated = (~sum(record[:-1]) + 1) & 0xFF
        if checksum != calculated:
            raise ValueError(f"Invalid Intel HEX at line {line_num}: bad checksum")

        if len(data) != byte_count:
            raise ValueError(f"Invalid Intel HEX at line {line_num}: length mismatch")

        if record_type == 0x00:  # Data record
            full_addr = extended_addr + addr
            if current_data and full_addr != current_base + len(current_data):
                # Non-contiguous, save current segment
                segments.append(IHexSegment(segment_start, bytes(current_data)))
                current_data = bytearray()
                segment_start = full_addr
                current_base = full_addr
            elif not current_data:
                segment_start = full_addr
                current_base = full_addr
            current_data.extend(data)

        elif record_type == 0x01:  # End of File
            break

        elif record_type == 0x02:  # Extended Segment Address
            if len(data) != 2:
                raise ValueError(f"Invalid Intel HEX at line {line_num}: bad type 02")
            extended_addr = ((data[0] << 8) | data[1]) << 4

        elif record_type == 0x04:  # Extended Linear Address
            if len(data) != 2:
                raise ValueError(f"Invalid Intel HEX at line {line_num}: bad type 04")
            extended_addr = ((data[0] << 8) | data[1]) << 16

        # Ignore other record types (03, 05)

    if current_data:
        segments.append(IHexSegment(segment_start, bytes(current_data)))

    return segments


def detect(content: bytes) -> bool:
    """Detect if content is Intel HEX format."""
    try:
        text = content[:256].decode("ascii")
        lines = [l.strip() for l in text.splitlines() if l.strip()]
        if not lines:
            return False
        # Check first few lines start with ':'
        for line in lines[:3]:
            if not line.startswith(":"):
                return False
            # Basic format check: should be hex chars after ':'
            if not all(c in "0123456789ABCDEFabcdef" for c in line[1:]):
                return False
        return True
    except (UnicodeDecodeError, ValueError):
        return False


def merge_segments(
    segments: list[IHexSegment], base_offset: int = 0
) -> tuple[int, bytes]:
    """Merge ihex segments into contiguous data with gap filling.

    Returns (start_address, data) where gaps are filled with 0x00.
    """
    if not segments:
        raise ValueError("No data in Intel HEX file")

    # Sort by address
    segments = sorted(segments, key=lambda s: s.address)

    start_addr = segments[0].address + base_offset
    end_addr = max(s.address + len(s.data) for s in segments) + base_offset

    # Create buffer filled with 0x00 (erased flash state)
    result = bytearray(end_addr - start_addr)

    # Copy each segment
    for seg in segments:
        offset = seg.address + base_offset - start_addr
        result[offset : offset + len(seg.data)] = seg.data

    return start_addr, bytes(result)
