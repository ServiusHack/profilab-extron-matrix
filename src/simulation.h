#pragma once

#include <boost/asio/io_service.hpp>

#include "configuration.h"
#include "device.h"

class Simulation
{
public:
  Simulation(const Configuration& configuration);
  ~Simulation();

  void Calculate(double* PInput, double* POutput, char** PStrings);

private:
  Configuration configuration;
  std::unique_ptr<Device> device;
  boost::asio::io_service io_service;
  std::unique_ptr<boost::asio::io_service::work> work;
  std::unique_ptr<std::thread> thread;

  std::mutex mutex;
  std::vector<unsigned int> previousNormalizedPInput;
  std::vector<std::string> previousInputNames;
  std::vector<std::string> previousOutputNames;
  std::vector<double> nextPOutput;
  std::string nextInputNames;
  std::string nextOutputNames;
  std::string errorMessage;
  bool canCommunicate{ false };
  size_t nextPOutputSizeInBytes = 0;
};
