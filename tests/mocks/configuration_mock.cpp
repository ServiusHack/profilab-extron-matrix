#include "configuration_mock.h"

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

ConfigurationMock configurationMockInstance;

Configuration::Configuration(double* PUser) {
  configurationMockInstance.Constructor(PUser, *this);
}

bool Configuration::Write() {
  return configurationMockInstance.Write();
}
