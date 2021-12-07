/**
 * @file   wave.cpp
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */

#include <ir/wave.h>

#include <stdexcept>
#include <thread>

using namespace ir;
using namespace std::chrono;

namespace {
struct wave_setup {
   wave_setup() { gpioWaveClear(); }
};
} // namespace

////////////////////////////////////////////////////////////////////////////////
/// Class: wave::carrier_parameters
////////////////////////////////////////////////////////////////////////////////
wave::carrier_parameters::carrier_parameters(double frequency_hz, double duty_cycle)
   : one_cycle_time{
      static_cast<unsigned>(static_cast<double>(duration_cast<microseconds>(seconds(1)).count()) / frequency_hz)}
   , on_state_duration{static_cast<std::uint32_t>(static_cast<double>(one_cycle_time.count()) * duty_cycle)}
   , off_state_duration{static_cast<std::uint32_t>(static_cast<double>(one_cycle_time.count()) * (1.0 - duty_cycle))} {
   // Nothing to do here
}

/**
 * Get a number of the square wave cycles in the given time duration.
 * @param duration Time duration in question.
 * @return Number of square wave cycles.
 */
unsigned wave::carrier_parameters::num_cycles(duration_t duration) const {
   return duration / one_cycle_time;
}

////////////////////////////////////////////////////////////////////////////////
/// Class: wave
////////////////////////////////////////////////////////////////////////////////
/**
 * Construct a wave.
 * @param pin_number Raspberry Pi pin number for the IR LED.
 * @param parameters Wave parameters.
 */
wave::wave(int pin_number, wave_parameters parameters)
   : pin_bit_{static_cast<std::uint32_t>(1 << pin_number)}
   , parameters_{parameters}
   , carrier_{parameters.frequency_hz, parameters.duty_cycle} {
   static wave_setup setup_waves;
}

wave::~wave() = default;

/**
 * Construct a square wave on the carrier frequency with a leading pulse burst and gap and an optional trailing burst
 */
void wave::build() {
   if (wave_id_ != PI_NO_WAVEFORM_ID) {
      throw std::runtime_error("Wave already constructed");
   }

   // Construct the wave from its components
   add_carrier_frequency(parameters_.leading_pulse);
   add_gap(parameters_.leading_gap);

   add_payload();

   if (parameters_.trailing_pulse.has_value()) {
      add_carrier_frequency(parameters_.trailing_pulse.value());
   }

   // Create a pigpio wave from the wave encoding
   gpioWaveAddGeneric(wave_.size(), wave_.data());
   wave_id_ = gpioWaveCreate();
   if (wave_id_ < 0) {
      throw std::runtime_error("Wave creation failure");
   }
}

/**
 * Send the wave using an IR LED.
 *
 * @note This operation is blocking and will return only when wave is sent.
 */
void wave::send() {
   int res = gpioWaveTxSend(wave_id_, PI_WAVE_MODE_ONE_SHOT);
   if (res == PI_BAD_WAVE_ID || res == PI_BAD_WAVE_MODE) {
      throw std::runtime_error("Error sending the wave");
   }

   while (gpioWaveTxBusy()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
}

/**
 * Add a square wave pulse burst of the given duration.
 * @param duration Pulse duration.
 */
void wave::add_carrier_frequency(duration_t duration) {
   const auto iterations = carrier_.num_cycles(duration);
   const auto on_duration = carrier_.on_state_duration;
   const auto off_duration = carrier_.off_state_duration;
   for (unsigned i = 0; i < iterations; ++i) {
      wave_.push_back(gpioPulse_t{.gpioOn = pin_bit_, .gpioOff = 0, .usDelay = on_duration});
      wave_.push_back(gpioPulse_t{.gpioOn = 0, .gpioOff = pin_bit_, .usDelay = off_duration});
   }
}

/**
 * Add a silence gap of the given duration.
 * @param duration Pulse duration.
 */
void wave::add_gap(duration_t duration) {
   const std::uint32_t pulse_duration = duration.count();
   wave_.push_back(gpioPulse_t{.gpioOn = 0, .gpioOff = 0, .usDelay = pulse_duration});
}

void wave::add_logical_zero() {
   if (parameters_.logical_zero.burst_first) {
      add_carrier_frequency(parameters_.logical_zero.burst_duration);
      add_gap(parameters_.logical_zero.gap_duration);
   } else {
      add_gap(parameters_.logical_zero.gap_duration);
      add_carrier_frequency(parameters_.logical_zero.burst_duration);
   }
}

void wave::add_logical_one() {
   if (parameters_.logical_one.burst_first) {
      add_carrier_frequency(parameters_.logical_one.burst_duration);
      add_gap(parameters_.logical_one.gap_duration);
   } else {
      add_gap(parameters_.logical_one.gap_duration);
      add_carrier_frequency(parameters_.logical_one.burst_duration);
   }
}
