#include "trompeloeil.hpp"

#include "device.h"

class DeviceMock {
 public:
  MAKE_MOCK4(Constructor,
             void(Device* self,
                  uint8_t inputs,
                  uint8_t outputs,
                  boost::asio::io_service& io_service));

  MAKE_MOCK2(tie, void(unsigned int input, unsigned int output));

  MAKE_MOCK2(audio_mute, void(unsigned int input, bool mute));

  MAKE_MOCK1(store, void(unsigned int index));

  MAKE_MOCK1(recall, void(unsigned int index));

  MAKE_MOCK1(open, void(const std::string& port_name));

  MAKE_MOCK0(close, void());

  MAKE_MOCK0(initialize, void());

  MAKE_MOCK1(read_banner_timeout, void(const boost::system::error_code& ec));

  MAKE_MOCK2(read_banner_handler,
             void(const boost::system::error_code& ec,
                  std::size_t bytes_transferred));

  MAKE_MOCK2(read_handler,
             void(const boost::system::error_code& ec,
                  std::size_t bytes_transferred));

  MAKE_MOCK0(send_next_message, void());

  MAKE_MOCK0(send_first_byte, void());
};

extern DeviceMock deviceMockInstance;
