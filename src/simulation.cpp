#include "simulation.h"

#include <boost/algorithm/string/find.hpp>
#include <cstring>

namespace {
unsigned int normalizeToUnsignedInt(double value) {
  return static_cast<unsigned int>(value);
}
}


Simulation::Simulation(const Configuration& configuration)
  : configuration(configuration) {

  previousNormalizedPInput.clear();
  previousNormalizedPInput.resize(2 + configuration.outputs,
                                  std::numeric_limits<unsigned int>::max());

  previousInputNames.clear();
  previousInputNames.resize(configuration.inputs);

  previousOutputNames.clear();
  previousOutputNames.resize(configuration.outputs);

  nextPOutput.clear();
  nextPOutput.resize(3 + configuration.outputs, 0.0);
  nextPOutputSizeInBytes = nextPOutput.size() * sizeof(double);

  nextInputNames = std::string(configuration.inputs - 1, ';');
  nextOutputNames = std::string(configuration.outputs - 1, ';');

  errorMessage = "";

  work = std::make_unique<boost::asio::io_service::work>(io_service);
  thread = std::make_unique<std::thread>([this]() { io_service.run(); });

  device = std::make_unique<Device>(io_service);
  try {
    device->connectedCallback = [this]() {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[0] = 5.0;
    };
    device->setupCallback = [this, configuration]() {
      std::unique_lock<std::mutex>(mutex);
      std::ostringstream str;
      if (device->get_number_of_virtual_outputs() != configuration.outputs)
        str << "Device has " << std::to_string(device->get_number_of_virtual_outputs()) << " outputs but DLL is configured for " << configuration.outputs << ".";
      if (device->get_number_of_virtual_inputs() != configuration.inputs)
        str << "Device has " << std::to_string(device->get_number_of_virtual_inputs()) << " inputs but DLL is configured for " << configuration.inputs << ".";

      const std::string message = str.str();
      if (!message.empty()) {
        nextPOutput[1] = 5.0;
        errorMessage = message;
      } else {
        canCommunicate = true;
      }
    };
    device->tieChanged = [this](uint8_t out, uint8_t in) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[3 + out - 1] = in;
    };
    device->inputNameChanged = [this](uint8_t channel, const std::string& name) {
      std::unique_lock<std::mutex>(mutex);
      auto begin = channel == 1
                       ? nextInputNames.begin()
                       : std::next(boost::algorithm::find_nth(nextInputNames, ";", channel - 2).begin());
      auto end = std::prev(boost::algorithm::find_nth(nextInputNames, ";", channel - 1).end());
      nextInputNames.replace(begin, end, name);
    };
    device->outputNameChanged = [this](uint8_t channel, const std::string& name) {
      std::unique_lock<std::mutex>(mutex);
      auto begin = channel == 1
                       ? nextOutputNames.begin()
                       : std::next(boost::algorithm::find_nth(nextOutputNames, ";", channel - 2).begin());
      auto end = std::prev(boost::algorithm::find_nth(nextOutputNames, ";", channel - 1).end());
      nextOutputNames.replace(begin, end, name);
    };
    device->reportError = [this](const std::string& message) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[1] = 5.0;
      errorMessage = message;
    };
    device->open(configuration.comPort);
  } catch (const boost::system::system_error& e) {
    nextPOutput[1] = 5.0;
    errorMessage = e.what();
  }
}

Simulation::~Simulation() {
  device->close();
  work.reset();
  if (thread->joinable())
    thread->join();
  thread.reset();
  device.reset();
}


void Simulation::Calculate(double* PInput, double* POutput, char** PStrings)
{
  std::unique_lock<std::mutex>(mutex);
  // We assume that PUser is the same as in the other calls. Therefore it's not
  // parsed every simulation step but the previously parsed configuration is
  // used.

  if (canCommunicate) {
    {
      const unsigned int normalizedStore = normalizeToUnsignedInt(PInput[0]);

      if (previousNormalizedPInput[0] != normalizedStore) {
        previousNormalizedPInput[0] = normalizedStore;

        if (normalizedStore != 0)
          device->store(normalizedStore);
      }
    }

    {
      const unsigned int normalizedRecall = normalizeToUnsignedInt(PInput[1]);

      if (previousNormalizedPInput[1] != normalizedRecall) {
        previousNormalizedPInput[1] = normalizedRecall;

        if (normalizedRecall != 0)
          device->recall(normalizedRecall);
      }
    }

    size_t offset = 2;  // store and recall from above

    for (unsigned int i = 0; i < configuration.outputs; i++) {
      const unsigned int normalizedValue = normalizeToUnsignedInt(PInput[offset + i]);
      if (previousNormalizedPInput[offset + i] != normalizedValue) {
        previousNormalizedPInput[offset + i] = normalizedValue;
        device->tie(normalizedValue, i + 1);  // i is 0-based but device parameters are 1-based
      }
    }

  offset += configuration.outputs;

 if (configuration.includeInputNames) {
   char* a = PStrings[offset];
   char* b = strstr(a, ";");
   size_t i = 0;
   while (a != nullptr && i < previousInputNames.size()) {
     size_t a_len = b == nullptr ? strlen(a) : static_cast<size_t>(b - a);
     if (strncmp(previousInputNames[i].c_str(), a, a_len) != 0) {
       previousInputNames[i] = std::string(a, a_len);
       device->set_input_name(i + 1, previousInputNames[i]);
     }
     ++i;
     a = b == nullptr ? nullptr : b + 1;
     b = a == nullptr ? nullptr : strstr(a, ";");
   }

      offset += 1;
    }

  if (configuration.includeOutputNames) {
    char* a = PStrings[offset];
    char* b = strstr(a, ";");
    size_t i = 0;
    while (a != nullptr && i < previousOutputNames.size()) {
      size_t a_len = b == nullptr ? strlen(a) : static_cast<size_t>(b - a);
      if (strncmp(previousOutputNames[i].c_str(), a, a_len) != 0) {
        previousOutputNames[i] = std::string(a, a_len);
        device->set_output_name(i + 1, previousOutputNames[i]);
      }
      ++i;
      a = b == nullptr ? nullptr : b + 1;
      b = a == nullptr ? nullptr : strstr(a, ";");
    }
  }

  }

  memcpy(POutput, nextPOutput.data(), nextPOutputSizeInBytes);

  memcpy(PStrings[2], errorMessage.data(), errorMessage.size() + 1);

  if (configuration.includeInputNames) {
    const size_t offset = 3 + configuration.outputs;
    memcpy(PStrings[offset], nextInputNames.data(), nextInputNames.size());
  }

  if (configuration.includeOutputNames) {
    const size_t offset = 3 + configuration.outputs + 1;
    memcpy(PStrings[offset], nextOutputNames.data(), nextOutputNames.size());
  }
}
