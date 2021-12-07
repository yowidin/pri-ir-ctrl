/**
 * @file   server.h
 * @author Dennis Sitelew
 * @date   Nov. 16, 2021
 */
#ifndef INCLUDE_IR_SERVER_H
#define INCLUDE_IR_SERVER_H

#include <memory>
#include <vector>
#include <cstdint>

#include <ir/wave.h>
#include <ir/led.h>
#include <ir/button.h>

#include <boost/asio.hpp>

namespace ir {

class server {
public:
   using wave_t = std::unique_ptr<wave>;
   using code_t = std::uint32_t;
   using wave_list_t = std::unordered_map<code_t, wave_t>;

   struct options {
      int ir_pin;
      int button_pin;
      int led_pin;
      code_t button_code;
   };

public:
   server(const options &options);

public:
   void run();

   void add_necx_wave(code_t code);

   void send_necx_wave(code_t code);

private:
   void handle_button_press();
   void do_accept(boost::asio::ip::tcp::acceptor &acceptor, boost::asio::ip::tcp::socket &socket);

private:
   options options_;

   led led_;
   button button_;
   wave_list_t waves_;

   boost::asio::io_context io_{};
};

} // namespace ir

#endif /* INCLUDE_IR_SERVER_H */
