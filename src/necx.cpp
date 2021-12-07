/**
 * @file   necx.cpp
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */

#include <ir/necx.h>
#include <chrono>

using namespace ir;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////
/// NECx Parameters
////////////////////////////////////////////////////////////////////////////////
namespace {

constexpr wave::wave_parameters nec_parameters{
   .frequency_hz = 38000,
   .duty_cycle = 0.5,
   .leading_pulse = milliseconds(9),
   .leading_gap = microseconds(4500),
   .logical_one = {.burst_duration = microseconds(562), .gap_duration = microseconds(1686), .burst_first = true},
   .logical_zero = {.burst_duration = microseconds(562), .gap_duration = microseconds(562), .burst_first = true},
   .trailing_pulse = microseconds(562)};

}

////////////////////////////////////////////////////////////////////////////////
/// Class: necx
////////////////////////////////////////////////////////////////////////////////
/**
 * Construct an extended NEC wave.
 * @param pin_number Raspberry Pi pin number for the IR LED.
 * @param code Binary NEC code (24 bits of address followed by 8 bits of code)
 */
necx::necx(int pin_number, std::uint32_t code)
   : wave{pin_number, nec_parameters}
   , code_{code} {
   build();
}

void necx::add_payload() {
   auto add_byte = [this](unsigned bits) {
      for (unsigned i = 0; i < 8; ++i) {
         if (bits & (1 << i)) {
            add_logical_one();
         } else {
            add_logical_zero();
         }
      }
   };

   add_byte(code_ >> 16);
   add_byte(code_ >> 8);
   add_byte(code_);
   add_byte(~code_);
}
