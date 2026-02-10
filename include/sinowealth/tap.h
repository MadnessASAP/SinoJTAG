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

#include <stddef.h>
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

  // --- DEBUG register (IR=0x02, 4-bit DR) ---
  class DEBUG {
    SimpleJTAG::Tap& tap_;
   public:
    DEBUG(SimpleJTAG::Tap& tap) : tap_(tap) {}
    static constexpr uint8_t HALT   = 0x01;
    static constexpr uint8_t ENABLE = 0x04;
    uint8_t operator()(uint8_t command);
  } DEBUG{*this};

  // --- CONFIG register (IR=0x03, 64-bit DR) ---
  class CONFIG {
    SimpleJTAG::Tap& tap_;
   public:
    CONFIG(SimpleJTAG::Tap& tap) : tap_(tap) {}
    struct __attribute__((packed)) write_t {
      uint8_t address : 7;
      uint16_t data : 16;
      uint64_t : (64 - 23);
    };

    struct __attribute__((packed)) read_t {
      uint8_t status : 4;
      uint8_t data : 8;
      uint64_t responses : 48;

      bool op_complete() const { return status & 0x01; }
      bool wait_extend() const { return status & 0x08; }
      uint8_t response(size_t index) const {
        return static_cast<uint8_t>((responses >> (index * 8)));
      }
    };
    uint64_t operator()(uint64_t data);
    read_t operator()(write_t data);
    read_t operator()(uint8_t addr = 0, uint16_t data = 0);
  } CONFIG{*this};

  // --- CODESCAN register (IR=0x00, 30-bit DR) ---
  // Fields are MSB-first: [15:0]=addr, [21:16]=ctrl, [29:22]=data
  class CODESCAN {
    SimpleJTAG::Tap& tap_;
   public:
    CODESCAN(SimpleJTAG::Tap& tap) : tap_(tap) {}
    struct __attribute__((packed)) fields_t {
      uint16_t address : 16;
      uint8_t ctrl : 6;
      uint8_t data : 8;
    };
    static constexpr uint8_t READ = 0x04;
    uint32_t operator()(uint32_t data);
    fields_t operator()(fields_t data);
    fields_t operator()(uint16_t address, uint8_t ctrl = READ);
  } CODESCAN{*this};

  // --- HALT register (IR=0x0C) ---
  class HALT {
    SimpleJTAG::Tap& tap_;
   public:
    HALT(SimpleJTAG::Tap& tap) : tap_(tap) {}
    void operator()();
    void operator()(uint8_t opcode);
  } HALT{*this};

  uint16_t IDCODE();

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
