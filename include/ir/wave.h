/**
 * @file   wave.h
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */
#ifndef INCLUDE_IR_WAVE_H
#define INCLUDE_IR_WAVE_H

#include <chrono>
#include <optional>
#include <vector>

#include <pigpio.h>

namespace ir {

/**
 * Represents a signal in pulse distance encoding.
 * Logical '0' and '1' are expected to be encoded as a sequence of signal silence bursts on the carrier frequency.
 * The carrier frequency is a square-wave signal with the adjustable duty cycle.
 */
class wave {
public:
   using duration_t = std::chrono::microseconds;

   //! Logical bit encoding
   struct bit_encoding {
      duration_t burst_duration; //!< Pulse burst duration
      duration_t gap_duration;   //!< Space duration
      bool burst_first;          //!< True in case if burst comes before space
   };

   //! Wave parameters description
   struct wave_parameters {
      double frequency_hz; //!< Carrier frequency

      //! Duty cycle for the carrier frequency (between 0 and 1).
      //! Smaller value will result in the longer 'off' state for each 'on' state in the encoding.
      //! 0.5 means that 'on' and 'off' states will be equal in duration:
      //!       _____      _____
      //!      |    |     |    |
      //!      |    |     |    |
      //! _____|    |_____|    |
      //! The 0.9 duty cycle will look something like this:
      //!   _________  _________
      //!  |        | |        |
      //!  |        | |        |
      //! _|        |_|        |
      //! The 0.1 duty cycle will look something like this:
      //!          ||         ||
      //!          ||         ||
      //!          ||         ||
      //! _________||_________||
      double duty_cycle;

      duration_t leading_pulse;
      duration_t leading_gap;

      bit_encoding logical_one;
      bit_encoding logical_zero;

      std::optional<duration_t> trailing_pulse;
   };

private:
   //! Stores internal information related to the carrier frequency and provides some convenience functionality
   struct carrier_parameters {
      explicit carrier_parameters(double frequency_hz, double duty_cycle);

      [[nodiscard]] unsigned num_cycles(duration_t duration) const;

      const duration_t one_cycle_time;        //!< Duration of a single square wave cycle
      const std::uint32_t on_state_duration;  //!< How long the IR LED stays ON for each square wave cycle, µs.
      const std::uint32_t off_state_duration; //!< How long the IR LED stays OFF for each square wave cycle, µs.
   };

public:
   wave(int pin_number, wave_parameters parameters);
   virtual ~wave();

   wave(const wave &) = default;
   wave(wave &&) = default;

public:
   virtual void send();
   virtual std::string name() const = 0;

protected:
   void add_carrier_frequency(duration_t duration);
   void add_gap(duration_t duration);
   void add_logical_zero();
   void add_logical_one();

   virtual void add_payload() = 0;

   void build();

private:
   //! The pigpio uses bit masks for pin state manipulations.
   //! Caches the RPi pin number -> pin bit conversion.
   const std::uint32_t pin_bit_;
   const wave_parameters parameters_;
   const carrier_parameters carrier_;

   //! pigpio wave identifier
   int wave_id_{PI_NO_WAVEFORM_ID};

   //! Wave encoding as a sequence of GPIO operations
   std::vector<gpioPulse_t> wave_{};
};

} // namespace ir

#endif /* INCLUDE_IR_WAVE_H */
