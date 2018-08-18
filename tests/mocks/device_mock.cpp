#include "device_mock.h"

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

DeviceMock deviceMockInstance;

Device::Device(boost::asio::io_service& io_service)
  : number_of_presets(32)
  , number_of_virtual_inputs(0)
  , number_of_virtual_outputs(0)
  , port(io_service)
  , buffer(2)
  , request_in_progress({ RequestType::None, "" })
  , viewed_current_outputs(0) {
  deviceMockInstance.Constructor(this, io_service);
}

uint8_t Device::get_number_of_virtual_inputs() const {
  return deviceMockInstance.get_number_of_virtual_inputs();
}

uint8_t Device::get_number_of_virtual_outputs() const {
  return deviceMockInstance.get_number_of_virtual_outputs();
}

void Device::tie(unsigned int input, unsigned int output) {
  deviceMockInstance.tie(input, output);
}

void Device::store(unsigned int index) {
  deviceMockInstance.store(index);
}

void Device::recall(unsigned int index) {
  deviceMockInstance.recall(index);
}

void Device::set_input_name(uint8_t index, const std::string& name) {
  deviceMockInstance.set_input_name(index, name);
}

void Device::set_output_name(uint8_t index, const std::string& name) {
  deviceMockInstance.set_output_name(index, name);
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
