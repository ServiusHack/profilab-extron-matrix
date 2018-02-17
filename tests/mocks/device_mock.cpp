#include "device_mock.h"

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

DeviceMock deviceMockInstance;

Device::Device(uint8_t inputs,
               uint8_t outputs,
               boost::asio::io_service& io_service)
    : number_of_inputs(0),
      number_of_outputs(0),
      port(io_service),
      buffer(10),
      request_last_sent_position(0),
      in_status_response(false),
      read_banner(true),
      banner_detection_timer(io_service) {
  deviceMockInstance.Constructor(this, inputs, outputs, io_service);
}

void Device::tie(unsigned int input, unsigned int output) {
  deviceMockInstance.tie(input, output);
}

void Device::audio_mute(unsigned int input, bool mute) {
  deviceMockInstance.audio_mute(input, mute);
}

void Device::store(unsigned int index) {
  deviceMockInstance.store(index);
}

void Device::recall(unsigned int index) {
  deviceMockInstance.recall(index);
}

void Device::open(const std::string& port_name) {
  deviceMockInstance.open(port_name);
}

void Device::close() {
  deviceMockInstance.close();
}

void Device::initialize() {
  deviceMockInstance.initialize();
}

void Device::add_to_queue(Request command) {}

void Device::read_banner_timeout(const boost::system::error_code& ec) {
  deviceMockInstance.read_banner_timeout(ec);
}

void Device::read_banner_handler(const boost::system::error_code& ec,
                                 std::size_t bytes_transferred) {
  deviceMockInstance.read_banner_handler(ec, bytes_transferred);
}

void Device::read_handler(const boost::system::error_code& ec,
                          std::size_t bytes_transferred) {
  deviceMockInstance.read_handler(ec, bytes_transferred);
}

void Device::send_next_message() {
  deviceMockInstance.send_next_message();
}

void Device::send_first_byte() {
  deviceMockInstance.send_first_byte();
}
