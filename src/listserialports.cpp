#include "listserialports.h"

#include <memory>

#include "Windows.h"

namespace {
const unsigned int upperProbeLimit = 128;

void DebugLogError(const char* port, DWORD errorCode)
{
  LPVOID formattedErrorCode;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&formattedErrorCode,
                0,
                NULL);

  std::string logMessage = std::string("GetDefaultCommConfig(") + port +
                           ") failed with error " + std::to_string(errorCode) +
                           ": " + std::string((LPSTR)formattedErrorCode);

  OutputDebugString(logMessage.c_str());

  LocalFree(formattedErrorCode);
}
} // namespace

std::vector<std::string> listSerialPorts()
{
  std::vector<std::string> existingPorts;

  for (unsigned int i = 1; i < upperProbeLimit; ++i) {
    char port[32] = { 0 };
    snprintf(port, sizeof(port), "COM%d", i);

    std::unique_ptr<COMMCONFIG> config(new COMMCONFIG);
    DWORD configSize = sizeof(COMMCONFIG);
    BOOL success = GetDefaultCommConfig(port, config.get(), &configSize);

    if (success) {
      existingPorts.emplace_back(port);
    } else {
      DWORD error = GetLastError();
      DebugLogError(port, error);
    }
  }

  return existingPorts;
}
