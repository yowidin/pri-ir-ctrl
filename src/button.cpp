/**
 * @file   button.cpp
 * @author Dennis Sitelew
 * @date   Nov. 08, 2021
 */

#include <ir/button.h>

#include <pigpio.h>

using namespace std;

ir::button::button(const int pin, callback_t cb, const duration_t debounce_interval)
   : pin_number_{pin}
   , callback_{std::move(cb)}
   , debounce_interval_{debounce_interval} {
   gpioSetMode(pin_number_, PI_INPUT);
   gpioSetPullUpDown(pin_number_, PI_PUD_UP);
   gpioSetAlertFuncEx(
      pin_number_,
      [](int gpio, int level, uint32_t tick, void *userdata) {
         reinterpret_cast<button *>(userdata)->handler(gpio, level, tick);
      },
      this);
}

ir::button::~button() {
   gpioSetAlertFuncEx(pin_number_, nullptr, this);
}

void ir::button::handler(int gpio, int level, uint32_t tick) {
   bool far_apart = (chrono::steady_clock::now() - last_press_time_) >= debounce_interval_;
   if (first_press_ || far_apart) {
      if (level == PI_LOW) {
         callback_();
      }
      first_press_ = false;
      last_press_time_ = chrono::steady_clock::now();
   }
}
