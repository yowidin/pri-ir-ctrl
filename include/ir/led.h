/**
 * @file   led.h
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */
#ifndef INCLUDE_IR_LED_H
#define INCLUDE_IR_LED_H

namespace ir {

class led final {
public:
   explicit led(int pin);

public:
   void turn_on();
   void turn_off();
   [[nodiscard]] bool is_on() const { return is_on_; }

private:
   const int pin_number_;
   bool is_on_{false};
};

class led_raii final {
public:
   explicit led_raii(led &led);
   ~led_raii();

private:
   led *led_;
};

} // namespace ir

#endif /* INCLUDE_IR_LED_H */
