#include "trompeloeil.hpp"

#include "device.h"

class DeviceMock {
 public:
  MAKE_MOCK2(Constructor,
             void(Device* self,
                  boost::asio::io_service& io_service));

  MAKE_MOCK0(get_number_of_virtual_inputs, uint8_t());

  MAKE_MOCK0(get_number_of_virtual_outputs, uint8_t());

  MAKE_MOCK2(tie, void(unsigned int input, unsigned int output));

  MAKE_MOCK1(store, void(unsigned int index));

  MAKE_MOCK1(recall, void(unsigned int index));

  MAKE_MOCK2(set_input_name, void(uint8_t index, const std::string& name));

  MAKE_MOCK2(set_output_name, void(uint8_t index, const std::string& name));

  MAKE_MOCK1(open, void(const std::string& port_name));

  MAKE_MOCK0(close, void());

  MAKE_MOCK0(initialize, void());
};

extern DeviceMock deviceMockInstance;
