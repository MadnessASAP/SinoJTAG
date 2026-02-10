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

#include <Arduino.h>

#include "rpc.h"
#include "sinowealth/phy.h"
#include "sinowealth/tap.h"
#include "sinowealth/icp.h"

auto _phy = sinowealth::Phy();
auto _tap = sinowealth::Tap();
auto _icp = sinowealth::ICP();

void setup() {
  rpc::setup();
}

void loop() {
  rpc::loop();
}


#if 0 // Debugging
#define LOG(msg) Serial.println(msg)
namespace {
  void print_mode() {
    Serial.print("MODE = 0x");
    Serial.println(
      static_cast<uint8_t>( _phy.mode() ),
      HEX
    );
  }

  auto mode(sinowealth::Phy::Mode mode) {
    LOG("PHY MODE"); _phy.mode(mode);
    print_mode();
  }
}

void setup() {
  Serial.begin(115200);

  LOG("PHY INIT"); _phy.init();
  print_mode();
  mode(sinowealth::Phy::Mode::JTAG);

  // IR/DR test
  LOG("TAP INIT"); _tap.init();
  LOG("TAP IDCODE");
  for (int i = 0; i < 4; ++i) {
    Serial.print("IDCODE: 0x");
    Serial.print(_tap.idcode(), HEX);
    Serial.print("\n");
  }

  // reset/enter test
  LOG("PHY RESET"); _phy.reset();
  print_mode();
  mode(sinowealth::Phy::Mode::JTAG);

  LOG("TAP INIT"); _tap.init();
  Serial.print("IDCODE: 0x");
  Serial.print(_tap.idcode(), HEX);
  Serial.print("\n");

  LOG("PHY STOP"); _phy.stop();
}

void loop() { }
#endif
