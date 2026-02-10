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

// --- ConfigReadDR ---

ConfigReadDR ConfigReadDR::from_raw(uint64_t raw) {
    ConfigReadDR r{};
    // status: bits 0,1 are low pair; bits 10,11 are high pair
    r.status = static_cast<uint8_t>(
        ((raw >> 0) & 0x03) | (((raw >> 10) & 0x03) << 2)
    );
    // data: bits 2-9
    r.data = static_cast<uint8_t>((raw >> 2) & 0xFF);
    // response: bits 16-63 (48 bits, 6 bytes)
    uint64_t resp = raw >> 16;
    for (uint8_t i = 0; i < 6; ++i) {
        r.response[i] = static_cast<uint8_t>(resp & 0xFF);
        resp >>= 8;
    }
    return r;
}

// --- CodescanDR ---

CodescanDR CodescanDR::from_raw(uint32_t raw) {
    CodescanDR r{};
    r.address = bit_reverse_16(static_cast<uint16_t>(raw & 0xFFFF));
    r.ctrl = static_cast<uint8_t>(bit_reverse_8(static_cast<uint8_t>((raw >> 16) & 0x3F) << 2));
    r.data = bit_reverse_8(static_cast<uint8_t>((raw >> 22) & 0xFF));
    return r;
}

// --- Tap methods ---

Status Tap::init() {
    goto_state(State::RunTestIdle);
    idle_clocks(2);

    // Step 1: Enable debug interface, unlock CONFIG register access
    IR(InstructionSet::DEBUG);
    DR<4>(static_cast<uint8_t>(DebugDR::ENABLE));
    idle_clocks(1);

    // Step 2: CONFIG register initialization
    IR(InstructionSet::CONFIG);

    // 2a: Enable debug/flash subsystem (needs ~50us settling)
    config_write(ConfigAddr::DEBUG_CTRL, DebugCtrlData::SUBSYS_ENABLE);
    _delay_us(50);

    // 2b: Full debug enable; arms flash erase capability
    config_write(ConfigAddr::DEBUG_CTRL, DebugCtrlData::DBGEN_FULL);

    // 2c: Clear DEBUG_CTRL register
    config_write(ConfigAddr::DEBUG_CTRL, DebugCtrlData::CLEAR);

    // Step 3: Clear target SFRs to known state
    // These map to SFRs at (addr + 0x80):
    //   0x63→P2CR, 0x67→PWMLO, 0x6B→P2PCR, 0x6F→P0OS,
    //   0x73→IB_CON2, 0x77→XPAGE, 0x7B→IB_OFFSET, 0x7F→debug_ctrl
    static constexpr uint8_t sfr_addrs[] = {
        0x63, 0x67, 0x6B, 0x6F,
        0x73, 0x77, 0x7B, 0x7F,
    };
    for (uint8_t i = 0; i < sizeof(sfr_addrs); ++i) {
        config_write(sfr_addrs[i], 0x0000);
    }

    // Step 4: Halt CPU
    IR(InstructionSet::DEBUG);
    DR<4>(static_cast<uint8_t>(DebugDR::HALT));
    idle_clocks(1);
    IR(InstructionSet::HALT);

    // Step 5: Enable flash debug access
    // Inject 8051 opcode: MOV 0xFF, #0x80 (opcode 0x75, addr 0xFF, imm 0x80)
    // SFR 0xFF bit 7 gates the flash debug interface
    opcode_inject(0x75);
    opcode_inject(0xFF);
    opcode_inject(0x80);

    // Step 6: Verify IDCODE
    uint16_t id = read_idcode();
    if (id == 0x0000 || id == 0xFFFF) {
        return Status::ERR_IDCODE;
    }

    return Status::OK;
}

void Tap::exit() {
    reset();
}

void Tap::config_write(uint8_t addr, uint16_t data) {
    DR<23>(static_cast<uint32_t>(ConfigDR{.data = data, .address = addr}));
    idle_clocks(1);
}

ConfigReadDR Tap::config_read_status() {
    // Arm readback by writing 0x0000 to STATUS_TRIGGER
    config_write(ConfigAddr::STATUS_TRIGGER, StatusTriggerData::CLEAR);
    // Read 64-bit status
    uint64_t raw = 0;
    DR<64, uint64_t>(uint64_t(0), &raw);
    return ConfigReadDR::from_raw(raw);
}

uint8_t Tap::codescan_read(uint16_t address) {
    IR(InstructionSet::CODESCAN);
    CodescanDR cmd{.address = address, .ctrl = CodescanDR::READ};
    uint32_t raw = 0;
    DR<30, uint32_t>(static_cast<uint32_t>(cmd), &raw);
    CodescanDR result = CodescanDR::from_raw(raw);
    return result.data;
}

void Tap::opcode_inject(uint8_t opcode) {
    // Must be in HALT state (IR=HALT already selected)
    // 8-bit partial scan into [29:22] of the 30-bit HALT register
    DR<8>(bit_reverse_8(opcode));
}

uint16_t Tap::read_idcode() {
    uint16_t id = 0;
    IR(InstructionSet::IDCODE);
    DR<16, uint16_t>(0, &id);
    return id;
}

} // namespace sinowealth
