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

 namespace jtag {
 /** JTAG TAP state enumeration. */
 enum class State : uint8_t {
   TestLogicReset = 0,
   RunTestIdle = 1,
   SelectDRScan = 2,
   CaptureDR = 3,
   ShiftDR = 4,
   Exit1DR = 5,
   PauseDR = 6,
   Exit2DR = 7,
   UpdateDR = 8,
   SelectIRScan = 9,
   CaptureIR = 10,
   ShiftIR = 11,
   Exit1IR = 12,
   PauseIR = 13,
   Exit2IR = 14,
   UpdateIR = 15,
 };

 constexpr State next_state(State s, bool tms) {
   switch (s) {
     case State::TestLogicReset:
       return tms ? State::TestLogicReset : State::RunTestIdle;
     case State::RunTestIdle:
       return tms ? State::SelectDRScan : State::RunTestIdle;
     case State::SelectDRScan:
       return tms ? State::SelectIRScan : State::CaptureDR;
     case State::CaptureDR:
       return tms ? State::Exit1DR : State::ShiftDR;
     case State::ShiftDR:
       return tms ? State::Exit1DR : State::ShiftDR;
     case State::Exit1DR:
       return tms ? State::UpdateDR : State::PauseDR;
     case State::PauseDR:
       return tms ? State::Exit2DR : State::PauseDR;
     case State::Exit2DR:
       return tms ? State::UpdateDR : State::ShiftDR;
     case State::UpdateDR:
       return tms ? State::SelectDRScan : State::RunTestIdle;
     case State::SelectIRScan:
       return tms ? State::TestLogicReset : State::CaptureIR;
     case State::CaptureIR:
       return tms ? State::Exit1IR : State::ShiftIR;
     case State::ShiftIR:
       return tms ? State::Exit1IR : State::ShiftIR;
     case State::Exit1IR:
       return tms ? State::UpdateIR : State::PauseIR;
     case State::PauseIR:
       return tms ? State::Exit2IR : State::PauseIR;
     case State::Exit2IR:
       return tms ? State::UpdateIR : State::ShiftIR;
     case State::UpdateIR:
       return tms ? State::SelectDRScan : State::RunTestIdle;
     default:
       return State::TestLogicReset;
   }
 }

 }
