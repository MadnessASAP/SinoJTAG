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
#include <stddef.h>

namespace sinowealth {

class ICP {
 public:
  /** Init ICP mode (800µs delay + ping). */
  void init();

  /** Transition from ICP to DIAG state (TCK high, TMS pulse). */
  void exit();

  /** Send a byte */
  void send_byte(uint8_t value);

  /** Receive a byte */
  uint8_t receive_byte();

  /** Send ICP ping command. */
  void ping();

  /** Verify ICP communication via readback test. */
  bool verify();

  /** Set the 16-bit flash address for subsequent operations. */
  void set_address(uint16_t address);

  /** Read flash memory into buffer. */
  void read_flash(uint16_t address, uint8_t* buffer, size_t size);

  /** Write flash memory from buffer. Enters ICP mode internally. Does NOT reset. */
  bool write_flash(uint16_t address, const uint8_t* buffer, uint16_t size);

  /** Erase flash sector at address. Enters/exits ICP mode internally. */
  bool erase_flash(uint16_t address);

  struct CommandSet {
    static constexpr uint8_t SET_IB_OFFSET_L = 0x40;
    static constexpr uint8_t SET_IB_OFFSET_H = 0x41;
    static constexpr uint8_t SET_IB_DATA     = 0x42;
    static constexpr uint8_t GET_IB_OFFSET   = 0x43;
    static constexpr uint8_t READ_FLASH      = 0x44;
    static constexpr uint8_t SET_EXTENDED    = 0x46;
    static constexpr uint8_t PING            = 0x49;
    static constexpr uint8_t READ_CUSTOM     = 0x4A;
    static constexpr uint8_t SET_XPAGE       = 0x4C;

    static constexpr uint8_t PREAMBLE[]    = { 0x15, 0x0A, 0x09, 0x06 };
    static constexpr uint8_t WRITE_UNLOCK  = 0x6E;
    static constexpr uint8_t ERASE_UNLOCK  = 0xE6;
    static constexpr uint8_t WRITE_TERM[]  = { 0x00, 0xAA, 0x00, 0x00 };
  };
};

}  // namespace sinowealth
