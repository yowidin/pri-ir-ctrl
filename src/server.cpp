/**
 * @file   server.cpp
 * @author Dennis Sitelew
 * @date   Nov. 16, 2021
 */

#include <ir/necx.h>
#include <ir/server.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/regex.hpp>

#include <pigpio.h>

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace ir;

namespace {

// https://github.com/facebook/folly/blob/main/folly/Uri.cpp
class uri {
public:
   using param_list_t = std::vector<std::pair<std::string, std::string>>;

public:
   static param_list_t get_query_params(beast::string_view text) {
      static const boost::regex query_param_regex(
              "(^|&)" /*start of query or start of parameter "&"*/
              "([^=&]*)=?" /*parameter name and "=" if value is expected*/
              "([^=&]*)" /*parameter value*/
              "(?=(&|$))" /*forward reference, next should be end of query or
                            start of next parameter*/);

      param_list_t result{};
      const boost::cregex_iterator begin(text.data(), text.data() + text.size(), query_param_regex);
      boost::cregex_iterator end;
      for (auto it = begin; it != end; ++it) {
         if (it->length(2) == 0) {
            // key is empty, ignore it
            continue;
         }
         result.emplace_back(std::string((*it)[2].first, (*it)[2].second), // parameter name
                             std::string((*it)[3].first, (*it)[3].second)  // parameter value
         );
      }
      return result;
   }
};

class http_connection : public std::enable_shared_from_this<http_connection> {
public:
   http_connection(server &server, tcp::socket socket)
      : server_{&server}
      , socket_(std::move(socket)) {
      // Nothing to do here
   }

public:
   void start() { read_request(); }

   void read_request() {
      auto self = shared_from_this();

      http::async_read(socket_, buffer_, request_, [self](auto ec, std::size_t) {
         if (!ec) {
            self->process_request();
         }
      });
   }

   void process_request() {
      response_.version(request_.version());
      response_.keep_alive(false);

      switch (request_.method()) {
         case http::verb::post:
            response_.result(http::status::ok);
            response_.set(http::field::server, "ir-ctrl");
            create_response();
            break;

         default:
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
               << "Invalid request-method '" << std::string(request_.method_string()) << "'";
            break;
      }

      write_response();
   }

   void create_response() {
      // Handle requests in the following form: (http://192.168.0.100/send?code=529287)
      beast::string_view prefix = "/send?";
      auto target = request_.target();
      if (!target.starts_with(prefix)) {
         response_.result(http::status::not_found);
         response_.set(http::field::content_type, "text/plain");
         beast::ostream(response_.body()) << "Unexpected request\r\n";
         return;
      }

      auto params = uri::get_query_params({target.data() + prefix.size(), target.size() - prefix.size()});
      // TODO: Handle different protocols
      for (const auto &p : params) {
         if (p.first == "code") {
            try {
               std::size_t pos = 0;
               auto code = std::stoi(p.second, &pos);
               if (pos != p.second.size()) {
                  throw std::invalid_argument("extra symbols");
               }

               server_->send_necx_wave(code);
            } catch (const std::invalid_argument &e) {
               response_.result(http::status::bad_request);
               response_.set(http::field::content_type, "text/plain");
               beast::ostream(response_.body()) << "Invalid IR code: " << e.what() << "\r\n";
            }
            break;
         }
      }
   }

   void write_response() {
      auto self = shared_from_this();

      response_.content_length(response_.body().size());

      http::async_write(socket_, response_,
                        [self](auto ec, std::size_t) { self->socket_.shutdown(tcp::socket::shutdown_send, ec); });
   }

private:
   server *server_;
   tcp::socket socket_;

   beast::flat_buffer buffer_{8192};
   http::request<http::dynamic_body> request_;
   http::response<http::dynamic_body> response_;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
/// Class: server::options
////////////////////////////////////////////////////////////////////////////////
result_t<server::options> server::options::load(int argc, char **argv) {
   namespace po = boost::program_options;

   po::options_description general("Raspberry Pi IR controller");
   general.add_options()("help,h", "Show help");

   po::options_description all;

   all.add_options()
      ("ir-pin", po::value<int>()->default_value(7), "IR sender-LED pin")
      ("button-pin", po::value<int>()->default_value(23), "Input button pin")
      ("led-pin", po::value<int>()->default_value(25), "LED button pin")
      ("button-code", po::value<std::uint32_t>()->default_value(0x81387), "IR code associated with a button press")
      ("listen-port", po::value<std::uint16_t>()->default_value(80), "HTTP-Server listen port");

   all.add(general);

   try {
      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, all), vm);

      if (vm.count("help")) {
         std::cout << all << "\n";
         return std::errc::interrupted;
      }

      po::notify(vm);

      auto ir_pin = vm["ir-pin"].as<int>();
      auto button_pin = vm["button-pin"].as<int>();
      auto led_pin = vm["led-pin"].as<int>();
      auto button_code = vm["button-code"].as<std::uint32_t>();
      auto listen_port = vm["listen-port"].as<std::uint16_t>();

      return options {ir_pin, button_pin, led_pin, button_code, listen_port};

   } catch (std::exception const &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << all << std::endl;
      return std::errc::invalid_argument;
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Class: server
////////////////////////////////////////////////////////////////////////////////
server::server(const options &options)
   : options_{options}
   , button_{options_.button_pin, [this] { handle_button_press(); }}
   , led_{options_.led_pin}
   , waves_{} {
   gpioSetMode(options_.ir_pin, PI_OUTPUT);

   add_necx_wave(options_.button_code);
}

void server::run() {
   tcp::acceptor acc{io_, {tcp::v4(), options_.listen_port}};
   tcp::socket sock{io_};

   // Handle signals
   boost::asio::signal_set signals(io_);
   signals.add(SIGINT);
   signals.add(SIGTERM);
   signals.async_wait([this](auto &ec, int signal) {
      if (ec) {
         std::cout << "Signal received:" << ec.message() << ", " << signal << std::endl;
      }
      io_.stop();
   });

   do_accept(acc, sock);
   io_.run();
}

void server::do_accept(boost::asio::ip::tcp::acceptor &acceptor, boost::asio::ip::tcp::socket &socket) {
   acceptor.async_accept(socket, [&](auto ec) {
      if (!ec) {
         std::make_shared<http_connection>(*this, std::move(socket))->start();
      } else {
         std::cerr << "Accept error: " << ec << std::endl;
      }
      do_accept(acceptor, socket);
   });
}

void server::add_necx_wave(code_t code) {
   waves_.insert({code, std::make_unique<ir::necx>(options_.ir_pin, code)});
}

void server::send_necx_wave(code_t code) {
   auto it = waves_.find(code);
   if (it == std::end(waves_)) {
      add_necx_wave(code);
   }

   std::cout << "HTTP send: 0x" << std::hex << code << "...";
   ir::led_raii raii(led_);
   waves_[code]->send();
   std::cout << "sent" << std::endl;
}

void server::handle_button_press() {
   std::cout << "Button press: 0x" << std::hex << options_.button_code << "...";
   ir::led_raii raii(led_);
   waves_[options_.button_code]->send();
   std::cout << "sent" << std::endl;
}
