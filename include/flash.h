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

class Iterator;

Iterator start(Tap& tap, uint16_t addr);

void end(Iterator& it);

/** Flash memory read iterator for SinoWealth devices. */
class Iterator {
 public:
  /**
    * Begin flash reading at the specified address.
    *
    * Selects the FLASH_ACCESS register, performs the initial garbage read,
    * and returns an iterator positioned at the first valid byte.
    */
  Iterator(Tap& tap, uint16_t addr);

  /**
   * End flash reading and reset the TAP.
   *
   * Invalidates the iterator and emits idle clocks to complete the operation.
   */
  ~Iterator();

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
  uint16_t address() const { return addr_ - 2; }

  /** Check if iterator is valid. */
  explicit operator bool() const { return tap_ != nullptr; }

 private:
  void read_next();

  Tap* tap_ = nullptr;
  uint16_t addr_ = 0;
  uint8_t data_ = 0;
};

}  // namespace flash
}  // namespace jtag
