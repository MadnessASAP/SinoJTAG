# SinoWealth JTAG Register Map

This document describes the JTAG instruction register (IR) values and their
associated data registers (DR) for SinoWealth 8051 microcontrollers (tested on SH79F6484).

IR width: 4 bits (values 0x0-0xF)

## Register Summary

| IR   | Name         | DR Width | Access | Description                     |
|------|--------------|----------|--------|---------------------------------|
| 0x0  | FLASH_ACCESS | 30-bit   | R      | Flash read / Status register    |
| 0x1  | BYPASS?      | 1-bit    | R      | Returns 0xFFFFFFFE (BYPASS-like)|
| 0x2  | CONTROL      | 4-bit    | R/W    | Control register                |
| 0x3  | DATA         | 23-bit   | R/W    | Data/address shift register     |
| 0x4-0xB | BYPASS?   | 1-bit    | R      | Return 0xFFFFFFFE (BYPASS-like) |
| 0xC  | EXIT         | ?        | W      | Halts CPU, enters debug mode    |
| 0xD  | BYPASS?      | 1-bit    | R      | Returns 0xFFFFFFFE              |
| 0xE  | IDCODE       | 16-bit   | R      | Device identification (0xC14C)  |
| 0xF  | BYPASS       | 1-bit    | R      | Standard JTAG bypass            |

## Detailed Register Descriptions

### IR 0x0: FLASH_ACCESS / STATUS

This register serves dual purpose:
1. **Flash access**: 30-bit protocol for reading flash memory
2. **Status reads**: 32-bit status when read with standard DR shift

#### Flash Read Protocol (from reference implementation)

```
1. Select IR = 0 (LSB-first, standard JTAG)
2. Navigate: Select-DR -> Capture-DR -> Shift-DR
3. Shift 16-bit address MSB-FIRST (not LSB-first!)
4. Shift 6 control bits: 0,0,0,1,0,0
5. Shift out 8 bits of data
6. Navigate: Exit1-DR -> Update-DR -> Idle -> Idle

Total DR length: 30 bits
First read returns garbage; data is for previous address.
```

**Important:** Flash reads require full postinit sequence first.

#### Status Register (32-bit LSB-first read)

**Observed behavior:**
- Before postinit: varies rapidly (CPU running)
- After EXIT (IR=12): becomes stable (CPU halted)
- Without postinit: stabilizes to `0x10040000`
- With postinit: stabilizes to `0x100F0000`

**Bit definitions:**

| Bit(s) | Mask       | Name      | Description                              |
|--------|------------|-----------|------------------------------------------|
| 28     | 0x10000000 | READY     | Set after first read, indicates valid    |
| 27     | 0x08000000 | INIT      | Set on first read before postinit        |
| 19     | 0x00080000 | BP_EN3?   | Set by postinit, breakpoint related      |
| 18     | 0x00040000 | ACTIVE    | Set after JTAG initialization            |
| 17     | 0x00020000 | BP_EN2?   | Set by postinit, breakpoint related      |
| 16     | 0x00010000 | BP_EN1?   | Set by postinit, breakpoint related      |
| 15-0   | 0x0000FFFF | DATA?     | Live data when CPU running, stable when halted |

### IR 0x2: CONTROL

4-bit control register. **Bit 0 must be set to maintain JTAG connection.**

**Critical finding from experimentation:**
```
Working values:  1, 3, 9, B  (0b0001, 0b0011, 0b1001, 0b1011)
Broken values:   0, 2, 4-8, A, C-F (cause STATUS to read 0x00000000)
```

**Bit definitions:**

| Bit | Value | Description                              |
|-----|-------|------------------------------------------|
| 3   | 0x8   | Unknown, can be set with bit 0           |
| 2   | 0x4   | Data register enable (set during setup)  |
| 1   | 0x2   | Unknown, can be set with bit 0           |
| 0   | 0x1   | **REQUIRED** - connection active         |

**Postinit sequence:**
1. Write `0b0100` (4) - enables DATA register operations
2. ... DATA register writes ...
3. Write `0b0001` (1) - normal operation mode

### IR 0x3: DATA

23-bit shift register for configuration and memory operations.

**Behavior:** This is a shift register - reads return the previously written value.

**Command format (tentative):**
```
Bits 22-16: Command/type field (7 bits)
Bits 15-0:  Address or data field (16 bits)
```

**Read commands (0x20-0x3C) - return 16-bit register values:**

| Command | Returns | Notes                    |
|---------|---------|--------------------------|
| 0x20    | varies  | Echoes set address       |
| 0x24    | 0x1256  | Fixed register value     |
| 0x28    | 0x9301  | Fixed register value     |
| 0x2C    | 0x28CB  | Fixed register value     |
| 0x30    | 0x2029  | Fixed register value     |
| 0x34    | 0x40B0  | Fixed register value     |
| 0x38    | 0x06B5  | Fixed register value     |
| 0x3C    | 0xCA8D  | Fixed register value     |

**Configuration commands (0x40xxxx):**
- `0x403000` - Initial configuration (requires 50µs delay after)
- `0x402000` - Configuration step 2
- `0x400000` - Configuration step 3 / Set address

**Breakpoint commands (0x63-0x7F, step 4):**
- `0x630000` - Breakpoint 0
- `0x670000` - Breakpoint 1
- `0x6B0000` - Breakpoint 2
- `0x6F0000` - Breakpoint 3
- `0x730000` - Breakpoint 4
- `0x770000` - Breakpoint 5
- `0x7B0000` - Breakpoint 6
- `0x7F0000` - Breakpoint 7

### IR 0xC: EXIT

Selecting this register halts the CPU and enters debug mode.

**Observed effect:**
- Before EXIT: STATUS register varies rapidly (CPU executing)
- After EXIT: STATUS becomes stable (e.g., `0x100F5F77`)

### IR 0xE: IDCODE

Standard JTAG identification register.

**Width:** 16-bit (non-standard; IEEE 1149.1 specifies 32-bit)

**Observed value:** `0xC14C` for SH79F6484

When read as 32-bit: `0xC14CC14C` (value repeated)

### IR 0x1, 0x4-0xB, 0xD: Unknown (BYPASS-like)

All return `0xFFFFFFFE` when read (all 1s except bit 0).
Likely unused registers defaulting to BYPASS behavior.

## Initialization Sequence (postinit)

Required before flash operations:

```
1. CONTROL (IR=2) <- 0b0100
2. DATA (IR=3) <- 0x403000, delay 50µs
3. DATA <- 0x402000
4. DATA <- 0x400000
5. DATA <- 0x630000 (BP0)
6. DATA <- 0x670000 (BP1)
7. DATA <- 0x6B0000 (BP2)
8. DATA <- 0x6F0000 (BP3)
9. DATA <- 0x730000 (BP4)
10. DATA <- 0x770000 (BP5)
11. DATA <- 0x7B0000 (BP6)
12. DATA <- 0x7F0000 (BP7)
13. CONTROL (IR=2) <- 0b0001
14. EXIT (IR=12)
```

After this sequence, STATUS becomes stable indicating CPU is halted.

## Flash Read Implementation

The 30-bit DR consists of:
- Bits 0-15: Address (MSB-first)
- Bits 16-21: Control sequence `0,0,0,1,0,0`
- Bits 22-29: Data output (MSB-first)

Since standard JTAG shifts LSB-first, we bit-reverse the MSB-first fields:

```cpp
uint16_t reverse16(uint16_t v) {
  v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
  v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
  v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
  v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
  return v;
}

uint8_t reverse8(uint8_t v) {
  v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0);
  v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC);
  v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA);
  return v;
}

uint8_t flash_read(uint16_t address) {
  tap.IR(0);  // Select FLASH_ACCESS

  // Control bits 0,0,0,1,0,0 MSB-first = 0x08 in bits 16-21
  uint32_t dr_out = reverse16(address) | (0x08UL << 16);
  uint32_t dr_in = 0;
  tap.DR<30>(dr_out, &dr_in);

  // Data is in bits 22-29, captured MSB-first
  uint8_t data = reverse8((dr_in >> 22) & 0xFF);

  tap.idle_clocks(2);
  return data;
}
```

**Key points:**
- Address and data are MSB-first; use bit reversal for standard LSB-first JTAG
- Control sequence `0,0,0,1,0,0` becomes `0x08` when packed into bits 16-21
- First byte read is garbage; data lags address by one
- Two idle cycles required after DR shift

## Timing Requirements

- 50 µs delay required after first DATA register write (0x403000)
- Idle clocks between register operations improve reliability
- Flash reads require ~30 clock cycles per byte

## Open Questions

- Exact meaning of 6-bit control sequence in flash read (0,0,0,1,0,0)
- How to write flash (erase/program operations)
- RAM access protocol
- Meaning of register values returned by 0x24-0x3C commands
- Why breakpoints use commands 0x63, 0x67... (pattern +4)
