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

#include "flash.h"

namespace {
inline uint16_t reverse16(uint16_t v) {
  v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
  v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
  v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
  v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
  return v;
}

inline uint8_t reverse8(uint8_t v) {
  v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0);
  v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC);
  v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA);
  return v;
}

}  // namespace detail


namespace jtag {
namespace flash {


/**
  * Begin flash reading at the specified address.
  *
  * Selects the FLASH_ACCESS register, performs the initial garbage read,
  * and returns an iterator positioned at the first valid byte.
  */
Iterator::Iterator(Tap& tap, uint16_t addr) : tap_(&tap), addr_(addr) {
  // Select FLASH_ACCESS register (IR=0)
  tap.IR(0);

  // 2 reads are needed to prime DR and data_
  //    it.addr_    = addr + 2
  //    DR          = addr + 1
  //    it.data_    = *addr
  read_next();
  read_next();
}

/**
 * End flash reading and reset the TAP.
 *
 * Invalidates the iterator and emits idle clocks to complete the operation.
 */
Iterator::~Iterator() {
  this->tap_->reset();
}

void Iterator::read_next() {
  // address word is 30 bits long
  // | 22222222 221111 11111100 00000000
  // | 98765432 109876 54321098 76543210
  // | read out 001000  address (MSB =>)

  // address sent MSB first
  uint32_t dr_out = reverse16(addr_);
  // insert mystery bits
  dr_out |= 0b001000UL << 16;

  uint32_t dr_in = 0;
  tap_->DR<30>(dr_out, &dr_in);
  // 2 idle clocks, definitley fails without this
  // first few reads will work but quickly starts to return garbage
  tap_->idle_clocks(2);

  // extract returned data
  dr_in >>= 22;
  dr_in &= 0xFF;
  data_ = reverse8(dr_in);
  ++addr_;
}

} // namespace jtag
} // namespace flash
