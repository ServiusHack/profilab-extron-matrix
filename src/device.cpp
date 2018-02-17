#include "device.h"

#include <iomanip>
#include <regex>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace {
unsigned char ReadyMarker[]{"READY\n"};
}

Device::Device(uint8_t inputs,
               uint8_t outputs,
               boost::asio::io_service& io_service)
    : number_of_inputs(inputs),
      number_of_outputs(outputs),
      port(io_service),
      buffer(10),
      request_last_sent_position(0),
      in_status_response(false),
      read_banner(true),
      banner_detection_timer(io_service) {}

void Device::tie(unsigned int input, unsigned int output) {
  assert(input && input <= number_of_inputs);
  assert(1 <= output && output <= number_of_outputs);

  std::stringstream str;
  if (input == 0)
    str << "DL" << level << "O" << output << "T";
  else
    str << "CL" << level << "I" << input << "O" << output << "T";
  add_to_queue({RequestType::Tie, str.str(), output, input});
}

void Device::audio_mute(unsigned int input, bool mute) {
  assert(input && input <= number_of_inputs);

  std::stringstream str;
  str << "CL" << level << "O" << input << "V" << (mute ? 'M' : 'U') << "T";
  add_to_queue({RequestType::AudioMute, str.str()});
}

void Device::store(unsigned int index) {
  assert(1 <= index && index <= 16);

  std::stringstream str;
  str << "RR" << index << "T";
  add_to_queue({RequestType::Store, str.str()});
}

void Device::recall(unsigned int index) {
  assert(1 <= index && index <= 16);

  std::stringstream str;
  str << "R" << index << "T";
  add_to_queue({RequestType::Recall, str.str()});
}

void Device::open(const std::string& port_name) {
  port.open(port_name);

  port.set_option(boost::asio::serial_port_base::baud_rate(9600));
  port.set_option(boost::asio::serial_port_base::character_size(8));
  port.set_option(boost::asio::serial_port_base::stop_bits(
      boost::asio::serial_port_base::stop_bits::one));
  port.set_option(boost::asio::serial_port_base::parity(
      boost::asio::serial_port_base::parity::none));
  port.set_option(boost::asio::serial_port_base::flow_control(
      boost::asio::serial_port_base::flow_control::none));

  // Start reading the banner from the serial port.
  banner_detection_timer.expires_from_now(boost::posix_time::seconds(1));
  banner_detection_timer.async_wait(boost::bind(
      boost::mem_fn(&Device::read_banner_timeout), boost::ref(*this), _1));
  port.async_read_some(boost::asio::buffer(buffer),
                       boost::bind(boost::mem_fn(&Device::read_banner_handler),
                                   boost::ref(*this), _1, _2));
}

void Device::close() {
  port.close();
}

void Device::initialize() {
  read_banner = false;

  port.async_read_some(boost::asio::buffer(buffer, 1),
                       boost::bind(boost::mem_fn(&Device::read_handler),
                                   boost::ref(*this), _1, _2));

  for (uint8_t output = 1; output <= number_of_outputs; ++output) {
    std::stringstream str;
    str << "SL" << level << "O" << static_cast<unsigned int>(output) << "T";
    add_to_queue({RequestType::Status, str.str(), output});
  }
}

void Device::add_to_queue(Request command) {
  std::lock_guard<std::mutex> lock_guard(request_queue_mutex);

  if (request_in_progress.type == RequestType::None) {
    request_in_progress = command;
    send_first_byte();
  } else {
    request_queue.push_back(std::move(command));
  }
}

void Device::read_banner_timeout(const boost::system::error_code& ec) {
  if (ec) {
    if (ec.value() == boost::asio::error::operation_aborted) {
      // Do nothing, the timer was aborted because something was read.
      return;
    } else {
      OutputDebugString(
          (boost::format("Banner read timeout error (%1% %2%): %3%") %
           ec.value() % ec.category().name() % ec.message())
              .str()
              .c_str());
      // Continue anways, maybe the error wasn't critical.
    }
  }

  port.cancel();
}

void Device::read_banner_handler(const boost::system::error_code& ec,
                                 std::size_t bytes_transferred) {
  banner_detection_timer.cancel();

  if (ec) {
    if (ec.value() == boost::asio::error::operation_aborted) {
      // Time has passed to wait for the banner. Start initializing.
      initialize();
    } else {
      OutputDebugString(
          (boost::format("Device communication error (%1% %2%): %3%") %
           ec.value() % ec.category().name() % ec.message())
              .str()
              .c_str());
      reportError("fatal read error");
    }
    return;
  }

  banner.insert(banner.end(), buffer.begin(),
                buffer.begin() + bytes_transferred);

  port.async_read_some(boost::asio::buffer(buffer),
                       boost::bind(boost::mem_fn(&Device::read_banner_handler),
                                   boost::ref(*this), _1, _2));

  if (std::find_end(banner.begin(), banner.end(), std::begin(ReadyMarker),
                    std::end(ReadyMarker)) != banner.end()) {
    OutputDebugString((std::string("Received banner:\n") + banner).c_str());
    initialize();
  }
}

void Device::read_handler(const boost::system::error_code& ec,
                          std::size_t bytes_transferred) {
  if (ec) {
    OutputDebugString(
        (std::string("Device communication error: ") + ec.message()).c_str());
    reportError("fatal read error");
    return;
  }

  if (bytes_transferred == 0)
    return;  // This seems to be the case when the port is closed, so we return
             // early.

  assert(bytes_transferred == 1);

  unsigned char first_byte = this->buffer[0];

  if (in_status_response) {
    if (first_byte == ')') {
      // end of status response
      in_status_response = false;

      // process accumulator
      std::stringstream sstr(accumulator);

      while (true) {
        unsigned int input;
        sstr >> input;

        if (sstr.eof())
          break;

        tieChanged(request_in_progress.output, static_cast<uint8_t>(input));
      }

      if (request_in_progress.output == number_of_outputs) {
        std::call_once(connectedCallbackOnceFlag, connectedCallback);
      }

      send_next_message();
    } else {
      accumulator.push_back(first_byte);
    }
  } else {
    if (first_byte == 'X' || first_byte == '?') {
      OutputDebugString(
          (boost::format(
               "Device communication error %1%%2% (complete request: %3%)") %
           request_in_progress.request.substr(0, request_last_sent_position) %
           first_byte % request_in_progress.request)
              .str()
              .c_str());
      send_next_message();
    } else if (first_byte == '(') {
      in_status_response = true;
      accumulator.clear();
    } else if (first_byte == 'T') {
      if (request_in_progress.type == RequestType::Tie)
        tieChanged(request_in_progress.output, request_in_progress.input);
      if (request_in_progress.type != RequestType::Status)
        send_next_message();
      // status response will follow with a '('.
    } else if (request_last_sent_position <
                   request_in_progress.request.length() &&
               first_byte ==
                   request_in_progress.request[request_last_sent_position]) {
      if (request_last_sent_position <
          request_in_progress.request.length() - 1) {
        port.write_some(boost::asio::buffer(
            &request_in_progress.request[++request_last_sent_position], 1));
      }
    } else {
      OutputDebugString((boost::format("Unrecognized response %1% after %2% "
                                       "characters of request %3%)") %
                         first_byte % (request_last_sent_position + 1) %
                         request_in_progress.request)
                            .str()
                            .c_str());
      reportError("unrecognized response");
    }
  }

  // Schedule the next read.
  if (port.is_open())
    port.async_read_some(boost::asio::buffer(buffer, 1),
                         boost::bind(boost::mem_fn(&Device::read_handler),
                                     boost::ref(*this), _1, _2));
}

void Device::send_next_message() {
  std::lock_guard<std::mutex> lock_guard(request_queue_mutex);

  if (request_queue.size() > 0) {
    request_in_progress = request_queue.front();
    request_queue.pop_front();
    send_first_byte();
  } else {
    request_in_progress = {RequestType::None, ""};
  }
}

void Device::send_first_byte() {
  request_last_sent_position = 0;
  port.write_some(boost::asio::buffer(
      &request_in_progress.request[request_last_sent_position], 1));
}
