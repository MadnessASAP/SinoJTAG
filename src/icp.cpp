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

#include "icp.h"
#include "config.h"
#include "phy.h"
#include "sinowealth.h"
#include <util/delay.h>

namespace jtag {
namespace icp {

void init() {
  _delay_us(800);       // TODO: Verify length of delay
  ping();
}

/** Exit is accomplished by holding TCK high while pualsing TMS, returns to diag mode */
void exit() {
  Phy::write_port(config::tck::port, config::tck::index, true);
  Phy::write_port(config::tms::port, config::tms::index, true);
  _delay_us(2);
  Phy::write_port(config::tms::port, config::tms::index, false);
  _delay_us(2);
}

void send_byte(uint8_t byte) {
  // bytes go out MSB
  byte = sinowealth::reverse8(byte);
  Phy::stream_bits<8, false>(byte);
  Phy::next_state(false);   // 1 extra clock pulse
}


uint8_t receive_byte() {
  // and come back LSB
  uint8_t byte = 0;
  Phy::stream_bits<8, false, uint8_t>(0, &byte);
  Phy::next_state(false);
  return byte;
}

void ping() {
  send_byte(cmd::PING);
  send_byte(0xFF);
}

bool verify() {
  set_address(0xFF69);

  send_byte(cmd::GET_IB_OFFSET);
  uint8_t b = receive_byte();
  (void)receive_byte();  // discard high byte

  return (b == 0x69);
}

void set_address(uint16_t address) {
  send_byte(cmd::SET_IB_OFFSET_L);
  send_byte(static_cast<uint8_t>(address & 0xFF));
  send_byte(cmd::SET_IB_OFFSET_H);
  send_byte(static_cast<uint8_t>((address >> 8) & 0xFF));
}

void read_flash(uint16_t address, uint8_t* buffer, uint8_t size) {
  set_address(address);
  // TODO: Support custom block
  send_byte(cmd::READ_FLASH);

  for (uint8_t n = 0; n < size; ++n) {
    buffer[n] = receive_byte();
  }
}

// bool write_flash(uint16_t address, const uint8_t* buffer, uint16_t size) {
//   if (size == 0) {
//     return false;
//   }

//   enter();
//   set_address(address);

//   send_byte(cmd::SET_IB_DATA);
//   send_byte(buffer[0]);

//   // Write initiation sequence
//   send_byte(0x6E);
//   send_byte(0x15);
//   send_byte(0x0A);
//   send_byte(0x09);
//   send_byte(0x06);

//   // Remaining data bytes with inter-byte delay
//   for (uint16_t n = 1; n < size; ++n) {
//     send_byte(buffer[n]);
//     _delay_us(5);
//     send_byte(0x00);
//   }

//   // Write termination sequence
//   send_byte(0x00);
//   send_byte(0xAA);
//   send_byte(0x00);
//   send_byte(0x00);
//   _delay_us(5);

//   return true;
// }

// bool erase_flash(uint16_t address) {
//   enter();
//   set_address(address);

//   send_byte(cmd::SET_IB_DATA);
//   send_byte(0x00);

//   // Erase initiation sequence
//   send_byte(0xE6);
//   send_byte(0x15);
//   send_byte(0x0A);
//   send_byte(0x09);
//   send_byte(0x06);

//   send_byte(0x00);
//   _delay_ms(300);
//   send_byte(0x00);
//   bool status = Phy::read_pin(config::tdo::pin, config::tdo::index);
//   send_byte(0x00);

//   reset();
//   return status;
// }

}  // namespace icp
}  // namespace jtag
