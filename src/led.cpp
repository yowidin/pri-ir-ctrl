/**
 * @file   led.cpp
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */

#include <ir/led.h>

#include <pigpio.h>

using namespace ir;

////////////////////////////////////////////////////////////////////////////////
/// Class: led
////////////////////////////////////////////////////////////////////////////////
led::led(int pin)
   : pin_number_{pin}
   , is_on_{false} {
   gpioSetMode(pin_number_, PI_OUTPUT);
   gpioSetPullUpDown(pin_number_, PI_PUD_DOWN);

   turn_off();
}

void led::turn_on() {
   is_on_ = true;
   gpioWrite(pin_number_, PI_HIGH);
}

void led::turn_off() {
   is_on_ = false;
   gpioWrite(pin_number_, PI_LOW);
}

////////////////////////////////////////////////////////////////////////////////
/// Class: led_raii
////////////////////////////////////////////////////////////////////////////////
led_raii::led_raii(led &led)
   : led_{&led} {
   led_->turn_on();
}

led_raii::~led_raii() {
   led_->turn_off();
}
