#include "simulation.h"

namespace {
unsigned int normalizeToUnsignedInt(double value) {
  return static_cast<unsigned int>(value);
}
}


Simulation::Simulation(const Configuration& configuration)
  : configuration(configuration) {

  previousNormalizedPInput.clear();
  previousNormalizedPInput.resize(2 + configuration.outputs, 0);

  previousInputNames.clear();
  previousInputNames.resize(configuration.inputs);

  previousOutputNames.clear();
  previousOutputNames.resize(configuration.outputs);

  nextPOutput.clear();
  nextPOutput.resize(3 + configuration.outputs, 0.0);
  nextPOutputSizeInBytes = nextPOutput.size() * sizeof(double);

  nextInputNames.clear();
  nextInputNames.resize(configuration.inputs);

  nextOutputNames.clear();
  nextOutputNames.resize(configuration.outputs);

  errorMessage = "";

  work = std::make_unique<boost::asio::io_service::work>(io_service);
  thread = std::make_unique<std::thread>([this]() { io_service.run(); });

  device = std::make_unique<Device>(io_service);
  try {
    device->open(configuration.comPort);
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
      }
    };
    device->tieChanged = [this](uint8_t out, uint8_t in) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[3 + out - 1] = in;
    };
    device->inputNameChanged = [this](uint8_t channel, const std::string& name) {
      std::unique_lock<std::mutex>(mutex);
      nextInputNames[channel] = name;
    };
    device->outputNameChanged = [this](uint8_t channel, const std::string& name) {
      std::unique_lock<std::mutex>(mutex);
      nextOutputNames[channel] = name;
    };
    device->reportError = [this](const std::string& message) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[1] = 5.0;
      errorMessage = message;
    };
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

  size_t offset = 2; //store and recall from above

  for (unsigned int i = 0; i < configuration.outputs; i++) {
    const unsigned int normalizedValue = normalizeToUnsignedInt(PInput[offset + i]);
    if (previousNormalizedPInput[offset + i] != normalizedValue) {
      previousNormalizedPInput[offset + i] = normalizedValue;
      if (normalizedValue != 0)
        device->tie(normalizedValue, i + 1); // i is 0-based but device parameters are 1-based
    }
  }

  offset += configuration.outputs;

  if (configuration.includeInputNames) {
    for (unsigned int i = 0; i < configuration.inputs; i++) {
      if (previousInputNames[i] != PStrings[offset + i]) {
        previousInputNames[i] = PStrings[offset + i];
        device->set_input_name(i + 1, previousInputNames[i]);
      }
    }

    offset += configuration.inputs;
  }

  if (configuration.includeOutputNames) {
    for (unsigned int i = 0; i < configuration.outputs; i++) {
      if (previousOutputNames[i] != PStrings[offset + i]) {
        previousOutputNames[i] = PStrings[offset + i];
        device->set_output_name(i + 1, previousOutputNames[i]);
      }
    }
  }

  memcpy(POutput, nextPOutput.data(), nextPOutputSizeInBytes);

  memcpy(PStrings[2], errorMessage.data(), errorMessage.size() + 1);

  if (configuration.includeInputNames) {
    const size_t offset = 3 + configuration.outputs;
    for (unsigned int i = 0; i < configuration.inputs; i++) {
      memcpy(PStrings[offset + i], nextInputNames[i].data(), nextInputNames[i].size() + 1);
    }
  }

  if (configuration.includeOutputNames) {
    const size_t offset = 3 + configuration.outputs + (configuration.includeInputNames ? configuration.inputs : 0);
    for (unsigned int i = 0; i < configuration.outputs; i++) {
      memcpy(PStrings[offset + i], nextOutputNames[i].data(), nextOutputNames[i].size() + 1);
    }
  }
}