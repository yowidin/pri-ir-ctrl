/**
 * @file   button.h
 * @author Dennis Sitelew
 * @date   Nov. 08, 2021
 */
#ifndef INCLUDE_IR_BUTTON_H
#define INCLUDE_IR_BUTTON_H

#include <chrono>
#include <functional>

namespace ir {

class button {
public:
   using callback_t = std::function<void()>;
   using time_point_t = std::chrono::steady_clock::time_point;
   using duration_t = std::chrono::milliseconds;

public:
   button(int pin, callback_t cb, duration_t debounce_interval = duration_t(50));
   ~button();

private:
   void handler(int gpio, int level, uint32_t tick);

private:
   const int pin_number_;
   const callback_t callback_;
   const duration_t debounce_interval_;

   bool first_press_{true};
   time_point_t last_press_time_{};
};

} // namespace ir

#endif /* INCLUDE_IR_BUTTON_H */
