#include "trompeloeil.hpp"

#include "configuration.h"

class ConfigurationMock {
 public:
  MAKE_MOCK2(Constructor, void(double* PUser, Configuration& configuration));
  MAKE_MOCK0(Write, bool());
};

extern ConfigurationMock configurationMockInstance;
