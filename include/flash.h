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

#include "tap.h"

namespace jtag {
namespace flash {

namespace detail {
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

template <int IR_BITS>
class Iterator;

template <int IR_BITS>
Iterator<IR_BITS> start(Tap<IR_BITS>& tap, uint16_t addr);

template <int IR_BITS>
void end(Iterator<IR_BITS>& it);

/** Flash memory read iterator for SinoWealth devices. */
template <int IR_BITS>
class Iterator {
 public:
  /** Return the byte at the current address. */
  uint8_t operator*() const { return data_; }

  /** Advance to the next address and return this iterator. */
  Iterator& operator++() {
    read_next();
    return *this;
  }

  /** Advance to the next address and return the previous iterator state. */
  Iterator operator++(int) {
    Iterator tmp = *this;
    read_next();
    return tmp;
  }

  /** Return the address of the current byte. */
  uint16_t address() const { return addr_ - 1; }

  /** Check if iterator is valid. */
  explicit operator bool() const { return tap_ != nullptr; }

 private:
  friend Iterator start<IR_BITS>(Tap<IR_BITS>& tap, uint16_t addr);
  friend void end<IR_BITS>(Iterator& it);

  void read_next() {
    // address word is 30 bits long
    // | 22222222 221111 11111100 00000000
    // | 98765432 109876 54321098 76543210
    // | read out 001000  address (MSB =>)

    // address sent MSB first
    uint32_t dr_out = detail::reverse16(addr_);
    // insert mystery bits
    dr_out |= 0b001000UL << 16;

    uint32_t dr_in = 0;
    tap_->template DR<30, uint32_t>(dr_out, &dr_in);
    // 2 idle clocks, definitley fails without this
    tap_->idle_clocks(2);
    data_ = detail::reverse8((dr_in >> 22) & 0xFF);
    ++addr_;
  }

  Tap<IR_BITS>* tap_ = nullptr;
  uint16_t addr_ = 0;
  uint8_t data_ = 0;
};

/**
 * Begin flash reading at the specified address.
 *
 * Selects the FLASH_ACCESS register, performs the initial garbage read,
 * and returns an iterator positioned at the first valid byte.
 */
template <int IR_BITS>
Iterator<IR_BITS> start(Tap<IR_BITS>& tap, uint16_t addr) {
  Iterator<IR_BITS> it;
  it.tap_ = &tap;

  // Select FLASH_ACCESS register (IR=0)
  tap.template IR<uint8_t>(0);

  it.addr_ = addr;
  // First read is bogus read_next twice causing
  //    it.addr_    = addr + 2
  //    DR          = addr + 1
  //    it.data_    = *addr
  it.read_next();
  it.read_next();
  return it;
}

/**
 * End flash reading and reset the TAP.
 *
 * Invalidates the iterator and emits idle clocks to complete the operation.
 */
template <int IR_BITS>
void end(Iterator<IR_BITS>& it) {
  if (it.tap_) {
    // it.tap_->IR(12);
    it.tap_ = nullptr;
  }
}

}  // namespace flash
}  // namespace jtag
