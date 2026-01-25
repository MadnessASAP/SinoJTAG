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


#include <SimpleJTAG/config.h>
#include <SimpleJTAG/phy.h>
#include <util/delay.h>

#include "sinowealth/phy.h"

namespace {
  using namespace config;
  /** Pre Vref IO setup */
  static inline void gpio_early_setup() {
    // All pins to input with pull-ups off
    vref::ddr &= ~_BV(vref::index);
    vref::port &= ~_BV(vref::index);
    tck::ddr &= ~_BV(tck::index);
    tck::port &= ~_BV(tck::index);
    tms::ddr &= ~_BV(tms::index);
    tms::port &= ~_BV(tms::index);
    tdi::ddr &= ~_BV(tdi::index);
    tdi::port &= ~_BV(tdi::index);
    tdo::ddr &= ~_BV(tdo::index);
    tdo::port &= ~_BV(tdo::index);
  }

  static inline void tck(bool state) {
    if (state) { tck::port |= _BV(tck::index); }
    else { tck::port &= ~_BV(tck::index); }
  }

  static inline void tms(bool state) {
    if (state) { tms::port |= _BV(tms::index); }
    else { tms::port &= ~_BV(tms::index); }
  }

  static inline void tdi(bool state) {
    if (state) { tdi::port |= _BV(tdi::index); }
    else { tdi::port &= ~_BV(tdi::index); }
  }

  static inline bool vref() { return vref::pin & _BV(vref::index); }
} // namespace

namespace sinowealth {

void Phy::init(bool wait_vref) {
  // skip if already initialized
  if (_mode != Mode::NOT_INITIALIZED) return;

  gpio_early_setup();

  if (wait_vref) {
    // Block on Vref, flash LED on PB5 to signal waiting
    DDRB |= _BV(5);   // PB5 output (LED)
    uint8_t count = 0;
    while (!::vref()) {
      if (++count == 0) {
        PORTB ^= _BV(5);  // Toggle LED every 256 iterations
      }
      _delay_us(200);
    }
    PORTB &= ~_BV(5);  // LED off
  }

  // Enable outputs
  SimpleJTAG::Phy::init();
  ::tck(true);
  ::tdi(true);
  ::tms(true);

  _delay_us(500);
  ::tck(false);
  _delay_us(1);
  ::tck(true);
  _delay_us(50);

  for (uint8_t n = 0; n < 165; ++n) {
    ::tms(false);
    _delay_us(2);
    ::tms(true);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 105; ++n) {
    ::tdi(false);
    _delay_us(2);
    ::tdi(true);
    _delay_us(2);
  }

  for (uint8_t n = 0; n < 90; ++n) {
    ::tck(false);
    _delay_us(2);
    ::tck(true);
    _delay_us(2);
  }

  for (uint16_t n = 0; n < 25600; ++n) {
    ::tms(false);
    _delay_us(2);
    ::tms(true);
    _delay_us(2);
  }

  _delay_us(8);
  ::tms(false);

  _mode = Mode::READY;
}

void Phy::stop() {
  SimpleJTAG::Phy::stop();
  _mode = Mode::NOT_INITIALIZED;
}

Phy::Mode Phy::mode(Mode mode) {
  if (_mode == mode || _mode == Mode::NOT_INITIALIZED) return _mode;

  // Have to be in ready state to switch modes
  if (_mode != Mode::READY) reset();

  // Mode byte is sent LSb with an extra 2 zero bits.
  uint16_t mode_packet = static_cast<uint8_t>(mode);
  Phy::stream_bits<10, false>(mode_packet);
  _mode = mode;

  return mode;
}

Phy::Mode Phy::reset() {
  // READY mode is held with TCK high and TMS low
  switch (_mode) {
    case Mode::JTAG:
      // 35 cycles with TMS high exits JTAG
      for (int i = 0; i < 35; ++i) Phy::next_state(true);
      ::tck(true);
      ::tms(false);
      _mode = Mode::READY;
      break;

    case Mode::ICP:
      // Pulsing TMS with clock high exits ICP
      ::tck(true);
      ::tms(true);
      delay_half();
      ::tms(false);
      delay_half();
      _mode = Mode::READY;
      break;

    default: break;
  }

  return _mode;
}

} // namespace sinowealth
