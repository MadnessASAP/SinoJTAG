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

/** Initialize UART0 for transmit-only logging at the given baud. */
void serial_init(uint32_t baud);

/** Blocking transmit of a single byte. */
void serial_write_byte(uint8_t data);

/** Write a null-terminated ASCII string. */
void serial_write_str(const char* text);

/** Write a 32-bit value as uppercase hexadecimal. */
void serial_write_hex32(uint32_t value);
