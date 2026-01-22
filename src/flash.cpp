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

// Explicit instantiation for 4-bit IR (SinoWealth)
template class jtag::flash::Iterator<4>;
template jtag::flash::Iterator<4> jtag::flash::start(jtag::Tap<4>&, uint16_t);
template void jtag::flash::end(jtag::flash::Iterator<4>&);
