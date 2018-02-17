#include "simulation.h"

namespace {
unsigned int normalizeToUnsignedInt(double value) {
  return static_cast<unsigned int>(value);
}
}  // namespace

Simulation::Simulation(const Configuration& configuration)
    : configuration(configuration) {
  previousNormalizedPInput.clear();
  previousNormalizedPInput.resize(2 + configuration.outputs, 0);

  nextPOutput.clear();
  nextPOutput.resize(3 + configuration.outputs, 0.0);
  nextPOutputSizeInBytes = nextPOutput.size() * sizeof(double);

  errorMessage = "";

  work = std::make_unique<boost::asio::io_service::work>(io_service);
  thread = std::make_unique<std::thread>([this]() { io_service.run(); });

  device = std::make_unique<Device>(configuration.inputs, configuration.outputs,
                                    io_service);
  try {
    device->open(configuration.comPort);
    device->connectedCallback = [this]() {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[0] = 5.0;
    };
    device->reportError = [this](const std::string& message) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[1] = 5.0;
      errorMessage = message;
    };
    device->tieChanged = [this](uint8_t out, uint8_t in) {
      std::unique_lock<std::mutex>(mutex);
      nextPOutput[3 + out - 1] = in;
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

void Simulation::Calculate(double* PInput, double* POutput, char** PStrings) {
  std::unique_lock<std::mutex>(mutex);
  // We assume that PUser is the same as in the other calls. Therefore it's not
  // parsed every simulation step but the previously parsed configuration is
  // used.

  {
    const unsigned int normalizedStorePin = normalizeToUnsignedInt(PInput[0]);

    if (previousNormalizedPInput[0] != normalizedStorePin) {
      previousNormalizedPInput[0] = normalizedStorePin;

      if (normalizedStorePin != 0) {
        device->store(normalizedStorePin);
      }
    }
  }

  {
    const unsigned int normalizedRecallPin = normalizeToUnsignedInt(PInput[1]);

    if (previousNormalizedPInput[1] != normalizedRecallPin) {
      previousNormalizedPInput[1] = normalizedRecallPin;

      if (normalizedRecallPin != 0) {
        device->recall(normalizedRecallPin);
      }
    }
  }

  const size_t offset = 2;  // store and recall from above

  for (unsigned int i = 0; i < configuration.outputs; i++) {
    const unsigned int normalizedValue =
        normalizeToUnsignedInt(PInput[offset + i]);
    if (previousNormalizedPInput[offset + i] != normalizedValue) {
      previousNormalizedPInput[offset + i] = normalizedValue;
      if (normalizedValue != 0)
        device->tie(normalizedValue,
                    i + 1);  // i is 0-based but device parameters are 1-based
    }
  }

  memcpy(POutput, nextPOutput.data(), nextPOutputSizeInBytes);

  memcpy(PStrings[2], errorMessage.data(), errorMessage.size() + 1);
}
