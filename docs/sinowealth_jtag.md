# SinoWealth JTAG Protocol Reference

Derived from decompilation of the SinoLink programmer firmware and confirmed
against the SH79F6484 target.

IR width: 4 bits (values 0x0-0xF)

## TAP Instructions

| IR   | Name     | DR Width | Description                              |
|------|----------|----------|------------------------------------------|
| 0x00 | CODESCAN | 30-bit   | Flash read (address/ctrl/data)           |
| 0x01 | BYPASS   | 1-bit    | BYPASS-like (returns 0xFFFFFFFE)         |
| 0x02 | DEBUG    | 4-bit    | Debug subsystem control                  |
| 0x03 | CONFIG   | 23/64-bit| Config/SFR access (write 23, read 64)    |
| 0x04 | RUN      | n/a      | Release CPU from halt                    |
| 0x05-0x0B | BYPASS | 1-bit | BYPASS-like (returns 0xFFFFFFFE)         |
| 0x0C | HALT     | 30-bit   | Halt CPU, opcode injection               |
| 0x0D | BYPASS   | 1-bit    | BYPASS-like                              |
| 0x0E | IDCODE   | 16-bit   | Device identification (e.g. 0xC14C)      |
| 0x0F | BYPASS   | 1-bit    | Standard JTAG bypass                     |

## DEBUG Register (IR=0x02, 4-bit DR)

| Value | Name   | Effect                                    |
|-------|--------|-------------------------------------------|
| 0x01  | HALT   | Halt CPU                                  |
| 0x04  | ENABLE | Enable debug interface, unlock CONFIG     |

## CONFIG Register (IR=0x03)

### Write: 23-bit DR

Bit layout (LSB-first): `[15:0] = data (16-bit), [22:16] = address (7-bit)`

### Address Space

| Address | Name           | Description                          |
|---------|----------------|--------------------------------------|
| 0x00    | STATUS_TRIGGER | Debug mode control / status readback |
| 0x40    | DEBUG_CTRL     | Debug/flash subsystem enable         |
| 0x63-0x7F | SFR range   | Maps to SFR at `(address + 0x80)`    |

### STATUS_TRIGGER Data Values (addr=0x00)

| Value  | Name         | Description                              |
|--------|--------------|------------------------------------------|
| 0x2000 | DBGEN_FULL   | Full debug enable; arms flash erase      |
| 0x1000 | DBGEN_SIMPLE | Reconnect shortcut                       |
| 0x0000 | CLEAR        | Clear; triggers status readback          |

### DEBUG_CTRL Data Values (addr=0x40)

| Value  | Name          | Description                             |
|--------|---------------|-----------------------------------------|
| 0x3000 | SUBSYS_ENABLE | Subsystem enable (needs ~50us settling) |
| 0x2000 | DBGEN_FULL    | Full debug enable; arms flash erase     |
| 0x0002 | FLASH_COMMIT  | Commit flash page config                |
| 0x0000 | CLEAR         | Clear register                          |

### Read: 64-bit DR

Irregular bit layout:

| Bits    | Field    | Width | Description                |
|---------|----------|-------|----------------------------|
| 0,1     | status_lo| 2-bit | Status bits (low pair)     |
| 2-9     | data     | 8-bit | Read data byte             |
| 10,11   | status_hi| 2-bit | Status bits (high pair)    |
| 12-15   | reserved | 4-bit | Unused                     |
| 16-63   | response | 48-bit| Extended response data     |

Status flags: bit 0 = op_complete, bit 3 = wait_extend.

### SFR Read Commands (addr 0x20-0x3C)

Addresses 0x20-0x3C in the CONFIG space return 16-bit SFR/register values:

| Address | Returns | Notes                    |
|---------|---------|--------------------------|
| 0x20    | varies  | Echoes set address       |
| 0x24    | 0x1256  | Fixed register value     |
| 0x28    | 0x9301  | Fixed register value     |
| 0x2C    | 0x28CB  | Fixed register value     |
| 0x30    | 0x2029  | Fixed register value     |
| 0x34    | 0x40B0  | Fixed register value     |
| 0x38    | 0x06B5  | Fixed register value     |
| 0x3C    | 0xCA8D  | Fixed register value     |

## CODESCAN Register (IR=0x00, 30-bit DR)

Fields are MSB-first (bit-reversed vs LSB-first JTAG shift order):

| Bits   | Field   | Width  | Description              |
|--------|---------|--------|--------------------------|
| 15:0   | address | 16-bit | Flash address (reversed) |
| 21:16  | ctrl    | 6-bit  | Control (reversed)       |
| 29:22  | data    | 8-bit  | Data byte (reversed)     |

Known ctrl values: `0x04` = READ.

Data lags address by one scan (first read returns garbage).

## HALT Register (IR=0x0C, 30-bit DR)

Selecting HALT halts the CPU. The 30-bit DR shares the same physical layout
as CODESCAN. The `[29:22]` (data/opcode) field accepts 8051 opcodes for
injection when shifting only 8 bits.

## IDCODE Register (IR=0x0E, 16-bit DR)

Non-standard width (IEEE 1149.1 specifies 32-bit). Returns `0xC14C` for
SH79F6484. When read as 32-bit: `0xC14CC14C` (value repeated).

## Initialization Sequence

1. **Enable debug**: IR=DEBUG, DR=ENABLE (0x04)
   - Enables debug interface, unlocks CONFIG register access

2. **Configure subsystem** (IR=CONFIG):
   - Write DEBUG_CTRL = SUBSYS_ENABLE (`{addr=0x40, data=0x3000}`), wait ~50us
   - Write DEBUG_CTRL = DBGEN_FULL (`{addr=0x40, data=0x2000}`)
   - Write DEBUG_CTRL = CLEAR (`{addr=0x40, data=0x0000}`)

3. **Clear SFRs to known state**: Write data=0x0000 to each address:

   | Address | SFR (addr+0x80) | Name       |
   |---------|-----------------|------------|
   | 0x63    | 0xE3            | P2CR       |
   | 0x67    | 0xE7            | PWMLO      |
   | 0x6B    | 0xEB            | P2PCR      |
   | 0x6F    | 0xEF            | P0OS       |
   | 0x73    | 0xF3            | IB_CON2    |
   | 0x77    | 0xF7            | XPAGE      |
   | 0x7B    | 0xFB            | IB_OFFSET  |
   | 0x7F    | 0xFF            | debug_ctrl |

4. **Halt CPU**: IR=DEBUG, DR=HALT (0x01); then IR=HALT

5. **Enable flash debug access**: Inject 8051 opcode `MOV 0xFF, #0x80`
   (bytes: 0x75, 0xFF, 0x80). SFR 0xFF bit 7 gates the flash debug interface.

6. **Verify IDCODE**: IR=IDCODE, read 16-bit DR. Reject 0x0000 and 0xFFFF.

## Timing Requirements

- ~50us delay required after SUBSYS_ENABLE write (subsystem power-up settling)
- Idle clocks (Run-Test/Idle) between register operations
- Flash reads via CODESCAN require ~30 clock cycles per byte
