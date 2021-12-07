#include <ir/server.h>

#include <iostream>
#include <cstddef>

#include <pigpio.h>

static const int BUTTON_INPUT_PIN = 23;
static const int BUTTON_LED_PIN = 25;
static const int IR_SENDER_PIN = 7;
static const std::uint32_t POWER_BUTTON = 0x81387;

namespace {

struct gpio_setup {
   gpio_setup() {
      if (gpioInitialise() < 0) {
         throw std::runtime_error("GPIO Initialization failed");
      }
   }

   ~gpio_setup() { gpioTerminate(); }
};

} // namespace


void main_unsafe() {
   static gpio_setup s_gpio_setup;

   ir::server server{ir::server::options{.ir_pin = IR_SENDER_PIN,
                                         .button_pin = BUTTON_INPUT_PIN,
                                         .led_pin = BUTTON_LED_PIN,
                                         .button_code = POWER_BUTTON}};

   server.run();
}

int main() {
   try {
      main_unsafe();
      return EXIT_SUCCESS;
   } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   } catch (...) {
      std::cerr << "Unknown exception" << std::endl;
      return EXIT_FAILURE;
   }
}