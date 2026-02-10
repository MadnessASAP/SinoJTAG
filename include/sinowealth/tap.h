/*
 * Copyright (C) 2026 Michael "ASAP" Weinrich
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

#pragma once

#include <stdint.h>
#include <SimpleJTAG/tap.h>

namespace sinowealth {

namespace detail {
    inline constexpr uint8_t bit_reverse_8(uint8_t v) {
        v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0);
        v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC);
        v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA);
        return v;
    }

    inline constexpr uint16_t bit_reverse_16(uint16_t v) {
        v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
        v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
        v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
        v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
        return v;
    }
}

// --- DEBUG register (IR=0x02, 4-bit DR) ---

struct DebugDR {
    uint8_t command = 0;
    static constexpr uint8_t HALT   = 0x01;
    static constexpr uint8_t ENABLE = 0x04;
    constexpr operator uint8_t() const { return command; }
};

// --- CONFIG write register (IR=0x03, 23-bit DR) ---
// Layout: [15:0]=data, [22:16]=address, all LSB-first

struct ConfigDR {
    uint16_t data = 0;
    uint8_t address = 0;
    constexpr operator uint32_t() const {
        return (static_cast<uint32_t>(address) << 16) | data;
    }
};

namespace ConfigAddr {
    static constexpr uint8_t STATUS_TRIGGER = 0x00;
    static constexpr uint8_t DEBUG_CTRL     = 0x40;
    // SFR range: address 0x63-0x7F maps to SFR (addr + 0x80)
}

namespace StatusTriggerData {
    static constexpr uint16_t DBGEN_FULL   = 0x2000;
    static constexpr uint16_t DBGEN_SIMPLE = 0x1000;
    static constexpr uint16_t CLEAR        = 0x0000;
}

namespace DebugCtrlData {
    static constexpr uint16_t SUBSYS_ENABLE = 0x3000;
    static constexpr uint16_t DBGEN_FULL    = 0x2000;
    static constexpr uint16_t FLASH_COMMIT  = 0x0002;
    static constexpr uint16_t CLEAR         = 0x0000;
}

// --- CONFIG read register (64-bit DR readback) ---
// Irregular bit layout from decompilation

struct ConfigReadDR {
    uint8_t status;       // 4-bit: bits 0,1,10,11 of raw
    uint8_t data;         // 8-bit: bits 2-9 of raw
    uint8_t response[6];  // 48-bit: bits 16-63 of raw

    bool op_complete() const { return status & 0x01; }
    bool wait_extend() const { return status & 0x08; }

    static ConfigReadDR from_raw(uint64_t raw);
};

// --- CODESCAN register (IR=0x00, 30-bit DR) ---
// Fields are MSB-first: [15:0]=addr, [21:16]=ctrl, [29:22]=data

struct CodescanDR {
    uint16_t address = 0;
    uint8_t ctrl = 0;
    uint8_t data = 0;

    static constexpr uint8_t READ = 0x04;

    constexpr operator uint32_t() const {
        uint32_t packed = 0;
        packed |= static_cast<uint32_t>(detail::bit_reverse_16(address));
        packed |= static_cast<uint32_t>(detail::bit_reverse_8(ctrl) >> 2) << 16;
        packed |= static_cast<uint32_t>(detail::bit_reverse_8(data)) << 22;
        return packed;
    }

    static CodescanDR from_raw(uint32_t raw);
};

// --- Status enum ---

enum class Status : uint8_t {
    OK = 0,
    ERR_IDCODE,
    ERR_FLASH_TIMEOUT,
};

// --- TAP class ---

class Tap : public SimpleJTAG::Tap {
 public:
  Status init();

  void config_write(uint8_t addr, uint16_t data);
  ConfigReadDR config_read_status();
  uint8_t codescan_read(uint16_t address);
  void opcode_inject(uint8_t opcode);
  uint16_t read_idcode();

  void exit();

  struct InstructionSet : SimpleJTAG::Tap::InstructionSet {
    static constexpr uint8_t CODESCAN = 0x00;
    static constexpr uint8_t DEBUG    = 0x02;
    static constexpr uint8_t CONFIG   = 0x03;
    static constexpr uint8_t RUN      = 0x04;
    static constexpr uint8_t HALT     = 0x0C;
  };
};

} // namespace sinowealth
