#include <assert.h>
#include <mutex>
#include <thread>

#include "configurationdialog.h"

#include <boost/format.hpp>

#include "simulation.h"

#define DLLEXPORT extern "C" __declspec(dllexport)

namespace {
const char* ApplicationName = "Extron-Matrix";

std::unique_ptr<Simulation> simulation;
Configuration configuration;
} // namespace

/**
 * Call by ProfiLab when the users wants to configure the DLL.
 */
DLLEXPORT void __stdcall CConfigure(double* PUser)
{
  ConfigurationDialog dlg(PUser);

  dlg.Get();

  if (!dlg.configuration.Write()) {
    MessageBox(
      NULL,
      "Configuration exceed 97 bytes. Unable to store it with ProfiLab.",
      ApplicationName,
      MB_OK | MB_ICONEXCLAMATION);
  }
}

/**
 * Called by ProfiLab to get the configured number of inputs.
 *
 * We read the configuration from PUser here because this is the first method
 * called after a DLL is loaded.
 */
DLLEXPORT unsigned char __stdcall CNumInputsEx(double* PUser)
{
  configuration = Configuration(PUser);
  if (configuration.present) {
    unsigned int numberOfInputs = configuration.outputs + 2;
    if (configuration.includeInputNames)
      numberOfInputs += 1;
    if (configuration.includeOutputNames)
      numberOfInputs += 1;
    assert(numberOfInputs <= std::numeric_limits<unsigned char>::max());
    return static_cast<unsigned char>(numberOfInputs);
  } else {
    // For some reason ProfiLab calls us without the PUser from the saved
    // circuit initially.
    // We just return 0 here, no harm done.
    return 0;
  }
}

/**
 * Called by ProfiLab to get the configured number of outputs.
 */
DLLEXPORT unsigned char __stdcall CNumOutputsEx(double* PUser)
{
  configuration = Configuration(PUser);

  if (configuration.present) {
    unsigned int numberOfOutputs = configuration.outputs + 3;
    if (configuration.includeInputNames)
      numberOfOutputs += 1;
    if (configuration.includeOutputNames)
      numberOfOutputs += 1;
    assert(numberOfOutputs <= std::numeric_limits<unsigned char>::max());
    return static_cast<unsigned char>(numberOfOutputs);
  } else {
    // For some reason ProfiLab calls us without the PUser from the saved
    // circuit initially.
    // We just return 0 here, no harm done.
    return 0;
  }
}

/**
 * Called by ProfiLab to get the name of an input channel.
 */
DLLEXPORT void __stdcall GetInputName(unsigned char Channel,
                                      unsigned char* Name)
{
  static const std::string storeInputName = "STORE";
  static const std::string recallInputName = "RECALL";
  static const std::string inputNames = "$INS";
  static const std::string outputNames = "$OUTS";

  switch (Channel) {
    case 0:
      memcpy(Name, storeInputName.c_str(), storeInputName.size() + 1);
      break;
    case 1:
      memcpy(Name, recallInputName.c_str(), recallInputName.size() + 1);
      break;
    default:
      Channel -= 2; // without the above inputs
      if (Channel < configuration.outputs) {
        // Casting is ok because the source string is only ASCII, so most
        // significant bit doesn't matter.
        sprintf(reinterpret_cast<char*>(Name), "OUT%d", Channel);
        return;
      }

      Channel -= configuration.outputs;

      if (configuration.includeInputNames) {
        if (Channel == 0) {
          memcpy(Name, inputNames.c_str(), inputNames.size() + 1);
          return;
        } else {
          --Channel;
        }
      }

      if (configuration.includeOutputNames) {
        if (Channel == 0) {
          memcpy(Name, outputNames.c_str(), outputNames.size() + 1);
          return;
        }
      }

      Name[0] = '\0';
  }
}

/**
 * Called by ProfiLab to get the name of an output channel.
 */
DLLEXPORT void __stdcall GetOutputName(unsigned char Channel,
                                       unsigned char* Name)
{
  static const std::string connectedOutputName = "CON";
  static const std::string errorOutputName = "ERR";
  static const std::string errorStringOutputName = "$ERR";
  static const std::string inputNames = "$INS";
  static const std::string outputNames = "$OUTS";

  switch (Channel) {
    case 0:
      memcpy(Name, connectedOutputName.c_str(), connectedOutputName.size() + 1);
      break;
    case 1:
      memcpy(Name, errorOutputName.c_str(), errorOutputName.size() + 1);
      break;
    case 2:
      memcpy(
        Name, errorStringOutputName.c_str(), errorStringOutputName.size() + 1);
      break;
    default:
      Channel -= 3; // without the above inputs
      if (Channel < configuration.outputs) {
        // Casting is ok because the source string is only ASCII, so most
        // significant bit doesn't matter.
        sprintf(reinterpret_cast<char*>(Name), "OUT%d", Channel);
        return;
      }

      Channel -= configuration.outputs;

      if (configuration.includeInputNames) {
        if (Channel == 0) {
          memcpy(Name, inputNames.c_str(), inputNames.size() + 1);
          return;
        } else {
          --Channel;
        }
      }

      if (configuration.includeOutputNames) {
        if (Channel == 0) {
          memcpy(Name, outputNames.c_str(), outputNames.size() + 1);
          return;
        }
      }

      Name[0] = '\0';
  }
}

/**
 * Called by ProfiLab when the simulation starts.
 */
DLLEXPORT void __stdcall CSimStart(double* /*PInput*/,
                                   double* /*POutput*/,
                                   double* PUser)
{
  const Configuration configuration = Configuration(PUser);

  simulation = std::make_unique<Simulation>(configuration);
}

/**
 * Called by ProfiLab for each simulation step (this will be a LOT of time).
 */
DLLEXPORT void __stdcall CCalculateEx(double* PInput,
                                      double* POutput,
                                      double* /*PUser*/,
                                      char** PStrings)
{
  simulation->Calculate(PInput, POutput, PStrings);
}

/**
 * Called by ProfiLab when the simulation ends.
 *
 * We do all the cleanup here. Doing so when the DLL is unloaded can lead to
 * deadlocks because
 * threads are destroyed.
 */
DLLEXPORT void __stdcall CSimStop(double* /*PInput*/,
                                  double* /*POutput*/,
                                  double* /*PUser*/)
{
  simulation.reset();
}

/**
 * Called when the DLL is loaded or unloaded.
 */
BOOL APIENTRY DllMain(HINSTANCE hInst /* Library instance handle. */,
                      DWORD reason /* Reason this function is being called. */,
                      LPVOID /*reserved*/ /* Not used. */)
{
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      ConfigurationDialog::dllInstance = hInst;
      break;

    case DLL_PROCESS_DETACH:
      ConfigurationDialog::dllInstance = NULL;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;
  }

  return TRUE;
}
