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

#include "serial.h"

#include <avr/io.h>

/** Write a 4-bit value as ASCII hex. */
static void serial_write_hex4(uint8_t value) {
  value &= 0x0F;
  serial_write_byte(
      static_cast<uint8_t>(value < 10 ? ('0' + value) : ('A' + (value - 10))));
}

void serial_init(uint32_t baud) {
  const uint16_t ubrr = static_cast<uint16_t>((F_CPU / (8UL * baud)) - 1U);
  UCSR0A = _BV(U2X0);
  UBRR0H = static_cast<uint8_t>(ubrr >> 8);
  UBRR0L = static_cast<uint8_t>(ubrr & 0xFF);
  UCSR0B = _BV(TXEN0);
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

void serial_write_byte(uint8_t data) {
  while ((UCSR0A & _BV(UDRE0)) == 0) {
  }
  UDR0 = data;
}

void serial_write_str(const char* text) {
  if (!text) {
    return;
  }
  while (*text) {
    serial_write_byte(static_cast<uint8_t>(*text++));
  }
}

void serial_write_hex32(uint32_t value) {
  for (int8_t shift = 28; shift >= 0; shift -= 4) {
    serial_write_hex4(static_cast<uint8_t>(value >> shift));
  }
}
