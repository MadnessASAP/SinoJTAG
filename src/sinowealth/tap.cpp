/*
 * Copyright (C) 2026 Michael "ASAP" Weinrich
 *
 * With inclusions from https://github.com/gashtaan/sinowealth-8051-dumper
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <util/delay.h>

#include "sinowealth/tap.h"

namespace sinowealth {

using detail::bit_reverse_8;
using detail::bit_reverse_16;

// --- CONFIG ---

uint64_t Tap::CONFIG::operator()(uint64_t data) {
    tap_.IR(uint8_t(0x03));
    uint64_t raw = 0;
    tap_.DR<64, uint64_t>(data, &raw);
    return raw;
}

Tap::CONFIG::read_t Tap::CONFIG::operator()(write_t w) {
    uint64_t out = (static_cast<uint64_t>(w.address) << 16 | w.data) << (64 - 23);
    uint64_t raw = (*this)(out);
    tap_.idle_clocks(1);

    read_t result{};
    result.status = raw & 0x0F;
    result.data = (raw >> 4) & 0xFF;
    result.responses = raw >> 12;
    return result;
}

Tap::CONFIG::read_t Tap::CONFIG::operator()(uint8_t addr, uint16_t data) {
    return (*this)(write_t{addr, data});
}

// --- DEBUG ---

uint8_t Tap::DEBUG::operator()(uint8_t command) {
    tap_.IR(uint8_t(0x02));
    uint8_t captured = 0;
    tap_.DR<4, uint8_t>(command, &captured);
    tap_.idle_clocks(1);
    return captured;
}

// --- CODESCAN ---

uint32_t Tap::CODESCAN::operator()(uint32_t data) {
    tap_.IR(uint8_t(0x00));
    uint32_t raw = 0;
    tap_.DR<30, uint32_t>(data, &raw);
    return raw;
}

Tap::CODESCAN::fields_t Tap::CODESCAN::operator()(fields_t f) {
    uint32_t out = 0;
    out |= static_cast<uint32_t>(bit_reverse_16(f.address));
    out |= static_cast<uint32_t>(bit_reverse_8(f.ctrl) >> 2) << 16;
    out |= static_cast<uint32_t>(bit_reverse_8(f.data)) << 22;

    uint32_t raw = (*this)(out);

    fields_t result{};
    result.address = bit_reverse_16(static_cast<uint16_t>(raw & 0xFFFF));
    result.ctrl = bit_reverse_8(static_cast<uint8_t>((raw >> 16) & 0x3F) << 2);
    result.data = bit_reverse_8(static_cast<uint8_t>((raw >> 22) & 0xFF));
    return result;
}

Tap::CODESCAN::fields_t Tap::CODESCAN::operator()(uint16_t address, uint8_t ctrl) {
    return (*this)(fields_t{address, ctrl, 0});
}

// --- HALT ---

void Tap::HALT::operator()() {
    tap_.IR(uint8_t(0x0C));
}

void Tap::HALT::operator()(uint8_t opcode) {
    tap_.IR(uint8_t(0x0C));
    tap_.DR<8>(bit_reverse_8(opcode));
}

// --- Tap methods ---

Status Tap::init() {
    goto_state(State::RunTestIdle);
    idle_clocks(2);

    // Step 1: Enable debug interface, unlock CONFIG register access
    DEBUG(DEBUG.ENABLE);

    // Step 2: CONFIG register initialization
    // 2a: Enable debug/flash subsystem (needs ~50us settling)
    CONFIG(ConfigAddr::DEBUG_CTRL, DebugCtrlData::SUBSYS_ENABLE);
    _delay_us(50);

    // 2b: Full debug enable; arms flash erase capability
    CONFIG(ConfigAddr::DEBUG_CTRL, DebugCtrlData::DBGEN_FULL);

    // 2c: Clear DEBUG_CTRL register
    CONFIG(ConfigAddr::DEBUG_CTRL, DebugCtrlData::CLEAR);

    // Step 3: Clear target SFRs to known state
    // These map to SFRs at (addr + 0x80):
    //   0x63→P2CR, 0x67→PWMLO, 0x6B→P2PCR, 0x6F→P0OS,
    //   0x73→IB_CON2, 0x77→XPAGE, 0x7B→IB_OFFSET, 0x7F→debug_ctrl
    static constexpr uint8_t sfr_addrs[] = {
        0x63, 0x67, 0x6B, 0x6F,
        0x73, 0x77, 0x7B, 0x7F,
    };
    for (uint8_t i = 0; i < sizeof(sfr_addrs); ++i) {
        CONFIG(sfr_addrs[i], 0x0000);
    }

    // Step 4: Halt CPU
    DEBUG(DEBUG.HALT);
    HALT();

    // Step 5: Enable flash debug access
    // Inject 8051 opcode: MOV 0xFF, #0x80 (opcode 0x75, addr 0xFF, imm 0x80)
    // SFR 0xFF bit 7 gates the flash debug interface
    HALT(0x75);
    HALT(0xFF);
    HALT(0x80);

    // Step 6: Verify IDCODE
    uint16_t id = IDCODE();
    if (id == 0x0000 || id == 0xFFFF) {
        return Status::ERR_IDCODE;
    }

    return Status::OK;
}

void Tap::exit() {
    reset();
}

uint16_t Tap::IDCODE() {
    IR(InstructionSet::IDCODE);
    uint16_t id = 0;
    DR<16, uint16_t>(0, &id);
    return id;
}

} // namespace sinowealth
