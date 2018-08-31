#include <catch.hpp>

#include "listserialports.h"

SCENARIO("listing serial ports", "[hardware-required]") {
  WHEN("getting list of serial ports") {
    std::vector<std::string> lists = listSerialPorts();

    THEN("there are serial ports") { REQUIRE(lists.size() > 0); }
  }
}
