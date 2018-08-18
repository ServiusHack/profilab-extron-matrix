#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <stdint.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

/**
 * @brief Interaction with the physical device.
 */
class Device
{

  // Basics
public:
  /**
   * @brief Construct a new instance.
   * @param inputs number of inputs the device has
   * @param outputs number of outputs the device has
   * @param io_service io service to read asynchronously from the serial port
   */
  explicit Device(boost::asio::io_service& io_service);

  uint8_t get_number_of_virtual_inputs() const;

  uint8_t get_number_of_virtual_outputs() const;

private:
  //! Number of presets the device supports.
  const uint8_t number_of_presets;
  //! Number of virtual inputs the device has.
  uint8_t number_of_virtual_inputs;
  //! Number of virtual outputs the device has.
  uint8_t number_of_virtual_outputs;

  // Device state
private:
  std::vector<uint8_t> current_input_of_output;
  std::vector<std::string> input_names;
  std::vector<std::string> output_names;

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

  //! Holds data when reading 16 bytes did not return a complete message
  std::vector<unsigned char> old_buffer;
  std::vector<unsigned char> buffer;

  // Protocol (message level)
public:
  //! The type of request to determine how to handle the response.
  enum class RequestType
  {
    None = 0,
    RequestInformation,
    BeginRequestCurrentConfiguration,
    RequestCurrentConfiguration,
    Tie,
    Store,
    Recall,
    ReadVirtualInputName,
    WriteVirtualInputName,
    ReadVirtualOutputName,
    WriteVirtualOutputName,
  };

  //! A request to send to the device.
  struct Request
  {

    // Remove when N3653 is available:
    Request(RequestType type, const std::string& request)
      : type(type)
      , request(request)
    {}

    Request(RequestType type, const std::string& request, uint8_t index)
      : type(type)
      , request(request)
      , index(index)
    {}

    //! The type of the request.
    RequestType type;
    //! The formatted request.
    std::string request;

    //! Only used for requesting names. Index of the input/output.
    uint8_t index{ 0 };
  };

private:
  enum class QueueType
  {
    HighPriority,
    LowPriority
  };

  //! Put requests into the request queue to read for all outputs the mapped
  //! inputs.
  void initialize();

  void request_begin_current_configuration_requests();
  void request_current_configuration(unsigned int start_output);

  void request_virtual_output_name(uint8_t output);
  void request_virtual_input_name(uint8_t input);

  //! Put a request into the request queue or directly execute it if there is no
  //! request in progress.
  void add_to_queue(Request command, QueueType queueType);

  void clear_queue(QueueType queueType);

  /**
   * @brief Process a complete response.
   * @param response the response from the device
   */
  void process_response(const std::string& response);

  //! Protect the request queue from concurrent access by the io service and
  //! zeromq.
  std::mutex request_queue_mutex;
  //! Queue of requests to send to the device.
  std::deque<Request> low_priority_request_queue;
  std::deque<Request> high_priority_request_queue;
  //! The request which is currently sent to the device or whose response is
  //! being read and processed.
  Request request_in_progress;

  //!
  uint8_t viewed_current_outputs;

  // Device interaction (RegieControlSystem level)
public:
  /**
   * @brief Map an audio and video input to an output.
   * @param input 1-based index of the input [1 <= input <= number_of_inputs]
   * @param output 1-based index of the output [1 <= output <=
   * number_of_outputs]
   */
  void tie(unsigned int input, unsigned int output);

  /**
   * @brief Store the current setup to a local preset.
   * @param index 1-based preset index
   */
  void store(unsigned int index);

  /**
   * @brief Recall the current setup from a local preset.
   * @param index 1-based preset index
   */
  void recall(unsigned int index);

  void set_input_name(uint8_t index, const std::string& name);
  void set_output_name(uint8_t index, const std::string& name);

  /**
   * @brief Callback being called when an video input was mapped to an output.
   */
  std::function<void(uint8_t out, uint8_t in)> tieChanged;

  /**
   * @brief Name of an input has changed.
   * @param input 1-based index of the input [1 <= input <= number_of_inputs]
   * @param name new name of the input
   */
  std::function<void(uint8_t input, std::string name)> inputNameChanged;

  /**
   * @brief Name of an output has changed.
   * @param input 1-based index of the output [1 <= input <= number_of_outputs]
   * @param name new name of the output
   */
  std::function<void(uint8_t output, std::string name)> outputNameChanged;

  /**
   * @brief Callback being called when the device is connected and initialized.
   * @see Device::initialize()
   */
  std::function<void()> connectedCallback;
  std::once_flag connectedCallbackOnceFlag;

  std::function<void()> setupCallback;
  std::once_flag setupCallbackOnceFlag;

  /**
   * @brief Callback when an error occurred.
   */
  std::function<void(const std::string& error)> reportError;
};
