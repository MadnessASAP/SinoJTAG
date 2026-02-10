/*
 * This file is derived from work done by Michal Kovacik at:
 * https://github.com/gashtaan/sinowealth-8051-dumper
 * https://github.com/gashtaan/sinowealth-8051-bl-updater
 *
 * Copyright (C) 2023, Michal Kovacik
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

#include <stddef.h>
#include <util/delay.h>
#include <SimpleJTAG/phy.h>

#include "sinowealth/phy.h"
#include "sinowealth/icp.h"

namespace {
  /** Flip the bits of a uint8_t */
  inline constexpr uint8_t bit_reverse(uint8_t v) {
    v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0);
    v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC);
    v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA);
    return v;
  }

  inline constexpr uint16_t bit_reverse(uint16_t v) {
    v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
    v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
    v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
    v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
    return v;
  }
}

namespace sinowealth {

void ICP::init() {
  _delay_us(800);       // TODO: Verify length of delay
  ping();
}

void ICP::send_byte(uint8_t byte) {
  // bytes go out MSb-first
  byte = bit_reverse(byte);
  Phy::stream_bits<8, false>(byte);
  Phy::next_state(false);   // 1 extra clock pulse
}


uint8_t ICP::receive_byte() {
  // and come back LSb-first
  uint8_t byte = 0;
  Phy::stream_bits<8, false, uint8_t>(0, &byte);
  Phy::next_state(false);
  return byte;
}

void ICP::ping() {
  send_byte(CommandSet::PING);
  send_byte(0xFF);
}

bool ICP::verify() {
  set_address(0xFF69);

  send_byte(CommandSet::GET_IB_OFFSET);
  uint8_t b = receive_byte();
  (void)receive_byte();  // discard high byte

  return (b == 0x69);
}

void ICP::set_address(uint16_t address) {
  send_byte(CommandSet::SET_IB_OFFSET_L);
  send_byte(static_cast<uint8_t>(address & 0xFF));
  send_byte(CommandSet::SET_IB_OFFSET_H);
  send_byte(static_cast<uint8_t>((address >> 8) & 0xFF));
}

void ICP::read_flash(uint16_t address, uint8_t* buffer, size_t size) {
  set_address(address);
  // TODO: Support custom block
  send_byte(CommandSet::READ_FLASH);

  for (size_t n = 0; n < size; ++n) {
    buffer[n] = receive_byte();
  }
}

bool ICP::write_flash(uint16_t address, const uint8_t* buffer, size_t size) {
  if (size == 0) {
    return false;
  }
  set_address(address);

  send_byte(CommandSet::SET_IB_DATA);
  send_byte(buffer[0]);

  // Write unlock sequence
  send_byte(CommandSet::WRITE_UNLOCK);
  for (auto b : CommandSet::PREAMBLE) {
    send_byte(b);
  }

  // Remaining data bytes with inter-byte delay
  for (size_t n = 1; n < size; ++n) {
    send_byte(buffer[n]);
    _delay_us(5);
    send_byte(0x00);
  }

  // Write termination sequence
  for (auto b : CommandSet::WRITE_TERM) {
    send_byte(b);
  }
  _delay_us(5);

  return true;
}

bool ICP::erase_flash(uint16_t address) {
  set_address(address);

  send_byte(CommandSet::SET_IB_DATA);
  send_byte(0x00);

  // Erase unlock sequence
  send_byte(CommandSet::ERASE_UNLOCK);
  for (auto b : CommandSet::PREAMBLE) {
    send_byte(b);
  }

  send_byte(0x00);
  _delay_ms(300);
  send_byte(0x00);
  bool status = Phy::read_pin(config::tdo::pin, config::tdo::index);
  send_byte(0x00);

  return status;
}

}  // namespace sinowealth
