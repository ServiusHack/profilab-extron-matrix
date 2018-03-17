#pragma once

#include <string>

/**
 * Configuration stored in PUser.
 */
struct Configuration {
  bool present{false};
  std::string comPort;
  unsigned int inputs{12};
  unsigned int outputs{12};
  bool includeInputNames{false};
  bool includeOutputNames{false};

  Configuration() = default;
  explicit Configuration(double* PUser);

  bool Write();

 private:
  char* user_data;
  static const size_t max_size = sizeof(double) * 100;
};
