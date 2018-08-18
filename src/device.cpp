#include "device.h"

#include <iomanip>
#include <regex>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
#include <sstream>

namespace {

namespace ResponsePatterns {
    const std::regex error("^E([0-9]{2})$");
    const std::regex tie("^Out([0-9]{2}) In([0-9]{2}) All$");
    // Documentation lies! U is followed by only one digit
    const std::regex request_information("^I([0-9]{2})X([0-9]{2}) T([0-9]) U([0-9]{1}) M([0-9]{2})X([0-9]{2}) Vmt([0-9]) Amt([0-9]) Sys([0-9]) Dgn([0-9]{2})$");
    const std::regex current_configuration("^([0-9]{2} ){16}All$");
    const std::regex reconfig("^RECONFIG([0-9]{2})$");
}

namespace Commands {
    const Device::Request request_information{Device::RequestType::RequestInformation, "I"};
}

}

Device::Device(boost::asio::io_service &io_service)
    : number_of_presets(32)
    , number_of_virtual_inputs(0)
    , number_of_virtual_outputs(0)
    , port(io_service)
    , buffer(2)
    , request_in_progress({RequestType::None, ""})
    , viewed_current_outputs(0)
{
}

uint8_t Device::get_number_of_virtual_inputs() const
{
    return number_of_virtual_inputs;
}

uint8_t Device::get_number_of_virtual_outputs() const
{
    return number_of_virtual_outputs;
}

void Device::tie(unsigned int input, unsigned int output)
{
    std::stringstream str;
    str << input << "*" << output << "!";
    const bool value_change = current_input_of_output[output] != input;
    add_to_queue({RequestType::Tie, str.str()}, value_change ? QueueType::HighPriority : QueueType::LowPriority);
}

void Device::store(unsigned int index)
{
    std::stringstream str;
    str << index << ",";
    clear_queue(QueueType::HighPriority);
    clear_queue(QueueType::LowPriority);
    add_to_queue({RequestType::Store, str.str()}, QueueType::HighPriority);
}

void Device::recall(unsigned int index)
{
    std::stringstream str;
    str << index << ".";
    clear_queue(QueueType::HighPriority);
    clear_queue(QueueType::LowPriority);
    add_to_queue({RequestType::Recall, str.str()}, QueueType::HighPriority);
}

void Device::set_input_name(uint8_t index, const std::string& name)
{
    std::stringstream str;
    str << "\x1BnI" << static_cast<unsigned int>(index) << "," << name << "\r";
    const bool value_change = input_names[index] != name;
    add_to_queue({RequestType::WriteVirtualInputName, str.str(), index}, value_change ? QueueType::HighPriority : QueueType::LowPriority);
}

void Device::set_output_name(uint8_t index, const std::string& name)
{
    std::stringstream str;
    str << "\x1BnO" << static_cast<unsigned int>(index) << "," << name << "\r";
    const bool value_change = output_names[index] != name;
    add_to_queue({RequestType::WriteVirtualOutputName, str.str(), index}, value_change ? QueueType::HighPriority : QueueType::LowPriority);
}

void Device::request_begin_current_configuration_requests()
{
    // Queue type must be the same as for the following request_* methods because this one must immediately preceed them.
    add_to_queue({RequestType::BeginRequestCurrentConfiguration, ""}, QueueType::LowPriority);
}

void Device::request_current_configuration(unsigned int start_output)
{
    std::stringstream str;
    str << "0*" << start_output << "*00VA";
    add_to_queue({RequestType::RequestCurrentConfiguration, str.str()}, QueueType::LowPriority);
}

void Device::request_virtual_output_name(uint8_t output)
{
    std::stringstream str;
    str << "\x1BNO" << static_cast<unsigned int>(output) << "\r";
    add_to_queue({RequestType::ReadVirtualOutputName, str.str(), output}, QueueType::LowPriority);
}

void Device::request_virtual_input_name(uint8_t input)
{
    std::stringstream str;
    str << "\x1BNI" << static_cast<unsigned int>(input) << "\r";
    add_to_queue({RequestType::ReadVirtualInputName, str.str(), input}, QueueType::LowPriority);
}

void Device::open(const std::string& port_name)
{
    port.open(port_name);

    port.set_option(boost::asio::serial_port_base::baud_rate(9600));
    port.set_option(boost::asio::serial_port_base::character_size(8));
    port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    initialize();
}

void Device::close()
{
    port.close();
}

void Device::initialize()
{
    // Start reading from the serial port.
    port.async_read_some(boost::asio::buffer(buffer), boost::bind(boost::mem_fn(&Device::read_handler), boost::ref(*this), _1, _2));

    add_to_queue(Commands::request_information, QueueType::LowPriority);
}

void Device::add_to_queue(Request command, QueueType queueType)
{
    std::lock_guard<std::mutex> lock_guard(request_queue_mutex);

    if (request_in_progress.type == RequestType::None)
    {
        request_in_progress = command;
        port.write_some(boost::asio::buffer(command.request));
    }
    else if (queueType == QueueType::HighPriority)
    {
        high_priority_request_queue.push_back(std::move(command));
    }
    else if (queueType == QueueType::LowPriority)
    {
        low_priority_request_queue.push_back(std::move(command));
    }
}

void Device::clear_queue(QueueType queueType)
{
    std::lock_guard<std::mutex> lock_guard(request_queue_mutex);

    switch (queueType)
    {
    case QueueType::HighPriority:
        high_priority_request_queue.clear();
        break;
    case QueueType::LowPriority:
        low_priority_request_queue.clear();
        break;
    }
}

void Device::process_response(const std::string& response)
{
    std::smatch m;
    std::regex_match(response, m, ResponsePatterns::error);
    if (!m.empty())
    {
        std::string error_message = (boost::format("Received %1% in response to %2%.") % response % request_in_progress.request).str().c_str();
        reportError(error_message);
    }
    else
    {
        {
            std::smatch m;
            std::regex_match(response, m, ResponsePatterns::reconfig);
            if (!m.empty())
            {
                unsigned int reconfig_id = boost::lexical_cast<unsigned int>(m.str(1));
                switch (reconfig_id) {
                  case 14:
                    // Connections changed
                    add_to_queue(Commands::request_information, QueueType::LowPriority);
                    break;
                  case 17:
                    // Name change for virtual input #1-16
                    for (uint8_t input = 1; input <= 16; ++input)
                    {
                      request_virtual_input_name(input);
                    }
                    break;
                  case 18:
                    // Name change for virtual input #17-32
                    for (uint8_t input = 17; input <= 32; ++input)
                    {
                      request_virtual_input_name(input);
                    }
                    break;
                  case 19:
                    // Name change for virtual input #33-48
                    for (uint8_t input = 33; input <= 48; ++input)
                    {
                      request_virtual_input_name(input);
                    }
                    break;
                  case 20:
                    // Name change for virtual input #49-64
                    for (uint8_t input = 49; input <= 64; ++input)
                    {
                      request_virtual_input_name(input);
                    }
                    break;
                  case 21:
                    // Name change for virtual output #1-16
                    for (uint8_t output = 1; output <= 16; ++output)
                    {
                      request_virtual_output_name(output);
                    }
                    break;
                  case 22:
                    // Name change for virtual output #17-32
                    for (uint8_t output = 17; output <= 32; ++output)
                    {
                      request_virtual_output_name(output);
                    }
                    break;
                  case 23:
                    // Name change for virtual output #33-48
                    for (uint8_t output = 33; output <= 48; ++output)
                    {
                      request_virtual_output_name(output);
                    }
                    break;
                  case 24:
                    // Name change for virtual output #49-64
                    for (uint8_t output = 49; output <= 64; ++output)
                    {
                      request_virtual_output_name(output);
                    }
                    break;
                }
                return;
            }
        }

        switch (request_in_progress.type)
        {
        case RequestType::RequestInformation:
        {
            std::smatch m;
            std::regex_match(response, m, ResponsePatterns::request_information);

            if (m.empty())
            {
                reportError("Unable to interpret the 'request information' response.");
            }
            else
            {
                unsigned int in_size = boost::lexical_cast<unsigned int>(m.str(1));
                unsigned int out_size = boost::lexical_cast<unsigned int>(m.str(2));
                unsigned int technology = boost::lexical_cast<unsigned int>(m.str(3));
                unsigned int number_of_units = boost::lexical_cast<unsigned int>(m.str(4));
                unsigned int in_map_size = boost::lexical_cast<unsigned int>(m.str(5));
                unsigned int out_map_size = boost::lexical_cast<unsigned int>(m.str(6));
                bool video_muted = boost::lexical_cast<unsigned int>(m.str(7)) == 1;
                bool audio_muted = boost::lexical_cast<unsigned int>(m.str(8)) == 1;
                unsigned int sys_power_supply_status = boost::lexical_cast<unsigned int>(m.str(9));
                unsigned int diagnostics = boost::lexical_cast<unsigned int>(m.str(10));

                {
                    std::ostringstream strm;
                    strm << "Received device information:";
                    strm << std::endl << "  " << in_size << " physical inputs";
                    strm << std::endl << "  " << out_size << " physical outputs";
                    switch (technology)
                    {
                        case 0:
                            strm << std::endl << "  BME not present";
                            break;
                        case 1:
                            strm << std::endl << "  wideband";
                            break;
                        case 2:
                            strm << std::endl << "  Lo-Res";
                            break;
                        case 3:
                            strm << std::endl << "  Sync";
                            break;
                        case 4:
                            strm << std::endl << "  Audio";
                            break;
                    }
                    strm << std::endl << "  " << number_of_units << " unit(s)";
                    strm << std::endl << "  " << in_map_size << " virtual inputs";
                    strm << std::endl << "  " << out_map_size << " virtual outputs";
                    strm << std::endl << "  " << (video_muted ? "video muted" : "video not muted");
                    strm << std::endl << "  " << (audio_muted ? "audio muted" : "audio not muted");
                    switch (sys_power_supply_status)
                    {
                        case 0:
                            strm << std::endl << "  off or dead power supply";
                            break;
                        case 1:
                            strm << std::endl << "  no redundant power supply, using main";
                            break;
                        case 2:
                            strm << std::endl << "  using redundant power supply";
                            break;
                        case 3:
                            strm << std::endl << "  has redundant power supply, using main";
                            break;
                    }
                    strm << std::endl << "  " << "Diagnostics code: " << diagnostics;

                    OutputDebugString(strm.str().c_str());
                }

                number_of_virtual_inputs = in_map_size;
                number_of_virtual_outputs = out_map_size;
                current_input_of_output.resize(number_of_virtual_outputs, 0);
                input_names.resize(number_of_virtual_inputs, "");
                output_names.resize(number_of_virtual_outputs, "");
                std::call_once(setupCallbackOnceFlag, setupCallback);

                request_begin_current_configuration_requests();
                for (unsigned int start_output = 1; start_output <= number_of_virtual_outputs; start_output += 16)
                {
                    request_current_configuration(start_output);
                }

                for (uint8_t output = 1; output <= number_of_virtual_outputs; ++output)
                {
                    request_virtual_output_name(output);
                }

                for (uint8_t input = 1; input <= number_of_virtual_outputs; ++input)
                {

                    request_virtual_input_name(input);
                }
            }

            break;
        }
        case RequestType::Tie:
        {
            std::smatch m;
            std::regex_match(response, m, ResponsePatterns::tie);

            if (m.empty())
            {
                reportError("Unable to interpret the 'tie' response.");
            }
            else
            {
                unsigned int out = boost::lexical_cast<unsigned int>(m.str(1));
                unsigned int in = boost::lexical_cast<unsigned int>(m.str(2));
                current_input_of_output[out] = in;
                tieChanged(out, in);
            }

            break;
        }
        case RequestType::RequestCurrentConfiguration:
        {
            std::smatch m;
            std::regex_match(response, m, ResponsePatterns::current_configuration);
            if(m.empty())
            {
                reportError("Unable to interpret the 'global preset ties' response.");
            }
            else
            {
                std::stringstream response_stream(response);
                for (unsigned int out = 1; out <= 16; ++out) {
                    unsigned int in;
                    response_stream >> in;

                    tieChanged(++viewed_current_outputs, static_cast<uint8_t>(in));

                    if (viewed_current_outputs >= number_of_virtual_outputs)
                    {
                        std::call_once(connectedCallbackOnceFlag, connectedCallback);
                        break;
                    }
                }
            }

            break;
        }
        case RequestType::ReadVirtualInputName:
        {
            input_names[request_in_progress.index - 1] = response;
            inputNameChanged(request_in_progress.index, response);
            break;
        }
        case RequestType::ReadVirtualOutputName:
        {
            output_names[request_in_progress.index - 1] = response;
            outputNameChanged(request_in_progress.index, response);
            break;
        }
        case RequestType::WriteVirtualInputName:
        {
          if (response != "NamI")
          {
            reportError((boost::format("Unexpected response '%1%' with request %2%") % response % request_in_progress.request).str());
          }
          request_virtual_input_name(request_in_progress.index);
          break;
        }
        case RequestType::WriteVirtualOutputName:
        {
          if (response != "NamO")
          {
            reportError((boost::format("Unexpected response '%1%' with request %2%") % response % request_in_progress.request).str());
          }
          request_virtual_output_name(request_in_progress.index);
          break;
        }
        default:
        {
            // Why did we get a response but did not expect one?
            reportError((boost::format("Unexpected response '%1%' with request %2%") % response % request_in_progress.request).str());
            break;
        }
        }
    }

    std::lock_guard<std::mutex> lock_guard(request_queue_mutex);

    auto queues = {&high_priority_request_queue, &low_priority_request_queue};

    for (auto& queue : queues)
    {
        if (queue->size() == 0)
            continue;

        request_in_progress = queue->front();
        queue->pop_front();

        if (request_in_progress.type == RequestType::BeginRequestCurrentConfiguration)
        {
            // Reset counter for which outputs we already processed RequestCurrentConfiguration.
            viewed_current_outputs = 0;

            // Read the next request, if one is already in the queue.
            if (queue->size() > 0)
            {
                request_in_progress = queue->front();
                queue->pop_front();
            }
            else
            {
                request_in_progress = {RequestType::None, ""};
                continue;
            }
        }

        port.write_some(boost::asio::buffer(request_in_progress.request));
        return;
    }

    // No queue contained any request.
    request_in_progress = {RequestType::None, ""};
}

void Device::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        reportError("Device communication error: " + ec.message());
        return;
    }

    if (bytes_transferred == 0)
        return; // This seems to be the case when the port is closed, so we return early.

    auto line_end = std::find(buffer.begin(), buffer.begin() + bytes_transferred, '\n');

    if (line_end != buffer.begin() + bytes_transferred) {
        old_buffer.insert(old_buffer.end(), buffer.begin(), line_end);
        process_response(std::string(old_buffer.begin(), old_buffer.end()-1)); // omit \r from old_buffer
        old_buffer = std::vector<unsigned char>(std::next(line_end), buffer.begin() + bytes_transferred);
    }
    else {
        old_buffer.insert(old_buffer.end(), buffer.begin(), buffer.begin() + bytes_transferred);
    }

    // Schedule the next read.
    if (port.is_open())
        port.async_read_some(boost::asio::buffer(buffer), boost::bind(boost::mem_fn(&Device::read_handler), boost::ref(*this), _1, _2));
}
