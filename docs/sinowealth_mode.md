<!--
Copyright (C) 2026 Michael "ASAP" Weinrich

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-->

# SinoWealth Special Mode and JTAG Entry (Observed + Inferred)

This note documents the observed "special" SinoWealth entry mode and
the transition into working JTAG, based on:
- `src/preinit.cpp` and `src/sinowealth_jtag.cpp` in this repo
- the local working reference at `../sinowealth-8051-dumper/`
- user bring-up experiments (January 2026)
- upstream README timing notes from the GitHub repo landing page citeturn0view0

The upstream GitHub web UI was returning "There was an error while loading"
for file views, so code-level comparisons against the online repo could not
be verified through the web tool. If you want those differences included,
provide a local checkout or specific files.

## Terminology

- **Power-on default**: target GPIOs are still at reset state (Hi-Z / default).
- **Special mode**: SinoWealth-specific state reached by a pre-init waveform.
- **JTAG mode**: normal TAP/JTAG operations after the mode byte and init sequence.

## Entry into special mode (preinit waveform)

The preinit waveform must be driven **before** configuring the JTAG pins for
normal operation. In SimpleJTAG this is `jtag::preinit()` called before PHY init.

Observed waveform (from `src/preinit.cpp` and the local reference dumper):

1) Wait for VREF high (no pull-up on VREF).
2) Configure TCK/TMS/TDI as outputs.
3) Drive TCK=TMS=TDI high.
4) Delay 500 us.
5) Pulse TCK low -> high with a short delay (1 us low, 50 us high).
6) Toggle TMS low/high **165** times with 2 us spacing.
7) Toggle TDI low/high **105** times with 2 us spacing.
8) Toggle TCK low/high **90** times with 2 us spacing.
9) Toggle TMS low/high **25600** times with 2 us spacing.
10) Delay 8 us.
11) Drive TMS low.

**Inference:** This waveform transitions the target from power-on to a
special "accepting mode" where the JTAG mode byte is honored. The exact
internal state is unknown.

## Transition from special mode to JTAG mode

Based on the local working dumper and SimpleJTAG bring-up:

1) **Send the mode byte (0xA5)**.
2) **Provide two extra TCK pulses** after the mode byte.
3) **Do NOT issue a long JTAG reset (35 TMS=1) here.**
4) Apply a short reset sequence: in the working dumper this is **8 TMS=1**
   clocks after the mode byte, not a full 35.
5) Run the JTAG init sequence (IR=2/3/12 and DR writes, see below).

**Bit order note (inferred):** The reference implementation shifts the mode
byte MSB-first, but 0xA5 is a bit palindrome so the JTAG mode byte does not
prove the order. It is equally valid to treat the byte as LSB-first as long
as the constants are defined in wire order (e.g., ICP mode would be 0x69
instead of 0x96). At the moment the code follows the reference MSB-first
convention for clarity. If you switch to LSB-first, update the constants.

**Inference:** A long reset (>= 14 TMS=1 clocks) appears to exit the
special mode or drop the target out of valid JTAG state. Empirically:
- 0..13 TMS=1 pulses: JTAG still works.
- >= 14 TMS=1 pulses: JTAG fails.

This behavior is consistent with the reference dumper using 8 pulses
instead of a full reset during its `JTAG::init()`.

## JTAG init sequence (SinoWealth-specific)

The working reference and SimpleJTAG use the following initialization
sequence after the mode byte:

1) IR = CONTROL_REG (2), DR<4> = 0b0100 (value 4)
2) IR = DATA_REG (3), DR<23> = 0x403000
3) Delay 50 us
4) DR<23> = 0x402000
5) DR<23> = 0x400000
6) Breakpoint init (all DR<23>):
   - 0x630000, 0x670000, 0x6B0000, 0x6F0000,
     0x730000, 0x770000, 0x7B0000, 0x7F0000
7) IR = CONTROL_REG (2), DR<4> = 0b0001 (value 1)
8) IR = EXIT (12)

Small idle gaps between DR writes are used in the local reference and
SimpleJTAG implementation (one extra idle cycle per DR).

## IDCODE

From the working path:
- IR = 14 (0xE)
- DR length = 16 bits
- The same device may be reported bit-reversed depending on shift/print order.

## Exit / reset behavior (inferred)

The reference dumper has two reset paths:
- **JTAG reset**: 35 cycles of TMS=1, then TMS=0.
- **ICP reset**: short TMS pulse sequence.

In SimpleJTAG, a full 35-cycle reset during/after the mode byte appears to
exit the special mode and makes JTAG IDCODE fail. This is not a standard
JTAG behavior and likely reflects SinoWealth-specific entry/exit rules.

## Timing sensitivity

The upstream README states the host must communicate with the target
within a few tens of milliseconds after power-up, implying the special
mode is only available in a short window.

## Suggested state model (inferred)

Power-on
-> Preinit waveform (special mode)
-> Mode byte (0xA5) + 2 extra clocks
-> Short reset (<= 13 TMS=1 pulses)
-> JTAG init sequence (IR/DR writes)
-> Normal JTAG operations

Failure cases:
- Missing extra clocks after 0xA5
- Using a long (>= 14) TMS=1 reset during init
- Exceeding the post-power-on timing window

## Open questions

- What is the exact cutoff (14 pulses) and why? Is it a command/escape
  for special mode rather than a JTAG reset?
- Is the timing window related to internal boot ROM or watchdog?
- Are there targets that require different init values or extra delays?
