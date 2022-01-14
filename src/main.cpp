#include <ir/server.h>

#include <cstddef>
#include <iostream>

#include <pigpio.h>

static const int BUTTON_INPUT_PIN = 23;
static const int BUTTON_LED_PIN = 25;
static const int IR_SENDER_PIN = 7;
static const std::uint32_t POWER_BUTTON = 0x81387;
static const std::uint16_t LISTEN_PORT = 80;

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

void main_unsafe(ir::server::options opts) {
   static gpio_setup s_gpio_setup;

   ir::server server{opts};
   server.run();
}

int main(int argc, char **argv) {
   try {
      auto opts = ir::server::options::load(argc, argv);
      if (!opts) {
         return EXIT_FAILURE;
      }

      main_unsafe(opts.value());
      return EXIT_SUCCESS;
   } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   } catch (...) {
      std::cerr << "Unknown exception" << std::endl;
      return EXIT_FAILURE;
   }
}