#pragma once

#include <stdint.h>
#include <deque>
#include <functional>
#include <mutex>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

/**
 * @brief Interaction with the physical device.
 *
 * The crosspoint targeted by this implementation is an AMX Modula system.
 *
 * The codes referes to something called "banner" which is a block of text
 * the Modula enclosure writes on the serial port when it is started. This
 * has to be read as no not be misinterpreted later as responses to requests.
 *
 * Requests are sent character by character to react to error responses
 * from the device. Its unclear if errors are only reported on "Take" (T
 * character)
 * so to be on the safe side we check it after every character.
 *
 * The words "route", "map" and "tie" are used interchangeably with the meaning
 * that an input port is connected to an output port such that the output
 * port outputs exactly what is coming into the input port.
 */
class Device {
  // Basics
 public:
  /**
   * @brief Construct a new instance.
   * @param inputs number of inputs of the Modula enclosure
   * @param outputs number of outputs of the Modula enclosure
   * @param io_service io service to read asynchronously from the serial port
   */
  Device(uint8_t inputs, uint8_t outputs, boost::asio::io_service& io_service);

 private:
  //! Virtual matrix (aka level) to switch on. Hardcoded value.
  unsigned int level = 1;
  //! Number of inputs the Modula enclosure has.
  const uint8_t number_of_inputs;
  //! Number of outputs the Modula enclosure has.
  const uint8_t number_of_outputs;

  // Device communication (serial port)
 public:
  /**
   * @brief Open a connection to the serial port.
   * @param port_name Path to the serial port, i.e. /dev/ttyUSB0.
   */
  void open(const std::string& port_name);

  /**
   * @brief Close the connection to the serial port.
   */
  void close();

 private:
  /**
   * @brief Called when bytes were read from the serial port.
   * @param ec error code
   * @param bytes_transferred number of bytes read
   */
  void read_handler(const boost::system::error_code& ec,
                    std::size_t bytes_transferred);
  //! The serial port through which to communicate with the device.
  boost::asio::serial_port port;
  //! Buffer for responses from the device. Except for reading the banner only
  //! the first byte will be used.
  std::vector<unsigned char> buffer;
  //! Accumulate status respones which are enclosed in parenthesis.
  std::string accumulator;

  // BCS Protocol (message level)
 private:
  //! The type of request to determine how to handle the response.
  enum class RequestType { None = 0, Status, Tie, AudioMute, Store, Recall };

  //! A request to send to the device.
  struct Request {
    //! The type of the request.
    RequestType type{RequestType::None};
    //! The formatted request.
    std::string request;
    //! The output addressed by the request. Only valid for type ==
    //! RequestType::Status and RequestType::Tie.
    unsigned int output{0};
    //! The output addressed by the request. Only valid for type ==
    //! RequestType::Tie.
    unsigned int input{0};

    // Remove when N3653 is available:
    Request(RequestType type = RequestType::None,
            std::string request = "",
            unsigned int output = 0,
            unsigned int input = 0)
        : type(type), request(request), output(output), input(input) {}
  };

  //! Put requests into the request queue to read for all outputs the mapped
  //! inputs.
  void initialize();
  //! Put a request into the request queue or directly execute it if there is no
  //! request in progress.
  void add_to_queue(Request command);
  //! Send the next request from the request queue.
  void send_next_message();
  //! Start to send the message by sending the first byte.
  void send_first_byte();

  //! Protect the request queue from concurrent access by the io service and
  //! zeromq.
  std::mutex request_queue_mutex;
  //! Queue of requests to send to the device.
  std::deque<Request> request_queue;
  //! The request which is currently sent to the device or whose response is
  //! being read and processed.
  Request request_in_progress;
  //! The position of the last character of the request sent to the device.
  size_t request_last_sent_position;
  //! True if a status response (enclosed in parenthesis) is currently read from
  //! the device.
  bool in_status_response;

  // Device interaction (RegieControlSystem level)
 public:
  /**
   * @brief Map an input to an output.
   * @param input 1-based index of the input [1 <= input <= number_of_inputs]
   * @param output 1-based index of the output [1 <= output <=
   * number_of_outputs]
   */
  void tie(unsigned int input, unsigned int output);

  /**
   * @brief Mute / unmute the audio of an input.
   * @param input 1-based index of the input [1 <= input <= number_of_inputs]
   * @param mute true if the audio of the input should be muted
   */
  void audio_mute(unsigned int input, bool mute);

  void store(unsigned int index);

  /**
   * @brief Recall the current setup from a local preset.
   * @param index 1-based preset index
   */
  void recall(unsigned int index);

  /**
   * @brief Callback being called when an input was mapped to an output.
   */
  std::function<void(uint8_t out, uint8_t in)> tieChanged;

  /**
   * @brief Callback being called when the device is connected and initialized.
   * @see Device::initialize()
   */
  std::function<void()> connectedCallback;
  std::once_flag connectedCallbackOnceFlag;

  /**
   * @brief Callback when an error occured.
   */
  std::function<void(const std::string& error)> reportError;

  // Banner processing
 private:
  /**
   * @brief Called when bytes were read from the serial port right after opening
   * it.
   * @param ec error code
   * @param bytes_transferred number of bytes read
   */
  void read_banner_handler(const boost::system::error_code& ec,
                           std::size_t bytes_transferred);
  /**
   * @brief Called when the time to wait for the banner is over.
   * @param ec error code from the timer
   *
   * This aborts reading the banner and continues with initializing the device.
   * @see Device::initialize()
   */
  void read_banner_timeout(const boost::system::error_code& ec);
  //! True if we are trying to read the banner.
  bool read_banner;
  //! Timer to abort waiting for the banner.
  boost::asio::deadline_timer banner_detection_timer;
  //! The complete banner read from the device.
  std::string banner;
};
