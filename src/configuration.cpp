#include "configuration.h"

Configuration::Configuration(double* PUser)
  : user_data(reinterpret_cast<char*>(PUser))
{
  present = user_data[0] == 1;
  if (present) {
    char* read_pointer = user_data + 1;
    comPort = std::string(read_pointer);

    read_pointer += comPort.size() + 1;

    memcpy(&inputs, read_pointer, sizeof(inputs));
    read_pointer += sizeof(inputs);
    memcpy(&outputs, read_pointer, sizeof(outputs));
    read_pointer += sizeof(outputs);

    includeInputNames = *read_pointer == 1;
    ++read_pointer;
    includeOutputNames = *read_pointer == 1;
  }
}

bool Configuration::Write()
{
  size_t data_size =
    1 + comPort.size() + 1 + sizeof(inputs) + sizeof(outputs) + 1 + 1;

  if (data_size > max_size) {
    return false;
  }

  char* write_pointer = user_data;

  memset(write_pointer, 0, data_size);

  memset(write_pointer, 1, 1);
  write_pointer += 1;
  memcpy(write_pointer, comPort.c_str(), comPort.size());
  write_pointer += comPort.size() + 1;
  memcpy(write_pointer, reinterpret_cast<void*>(&inputs), sizeof(inputs));
  write_pointer += sizeof(inputs);
  memcpy(write_pointer, reinterpret_cast<void*>(&outputs), sizeof(outputs));
  write_pointer += sizeof(outputs);

  *write_pointer = includeInputNames ? 1 : 0;
  write_pointer += 1;
  *write_pointer = includeOutputNames ? 1 : 0;
  return true;
}
