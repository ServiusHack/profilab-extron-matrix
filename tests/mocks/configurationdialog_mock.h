#define TROMPELOEIL_LONG_MACROS
#include "trompeloeil.hpp"

class ConfigurationDialogMock {
 public:
  TROMPELOEIL_MAKE_MOCK0(Get, void());
};

extern ConfigurationDialogMock configurationDialogMockInstance;
