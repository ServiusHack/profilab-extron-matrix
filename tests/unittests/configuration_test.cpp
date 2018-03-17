#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <array>

#include "configuration.h"

static_assert(sizeof(double) == 8, "Tests assume 8 byte doubles");
static_assert(sizeof(unsigned int) == 4, "Tests assume 5 byte unsigned ints");

SCENARIO("serialization of the configuration", "[configuration]") {
  GIVEN("Default-constructed configuration") {
    Configuration configuration;
    THEN("The configuration is marked as not present") {
      REQUIRE_FALSE(configuration.present);
    }
  }

  GIVEN("No stored configuration") {
    std::array<unsigned char, 1 * 8> data{0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00};

    double* PUser = reinterpret_cast<double*>(data.data());

    WHEN("deserializing the configuration") {
      Configuration configuration(PUser);

      THEN("The configuration is marked as not present") {
        REQUIRE_FALSE(configuration.present);
      }
    }
  }

  GIVEN("A serialized configuration") {
    std::array<unsigned char, 1 + 5 + 2 * 4> data{
        0x01,                          // present
        'C',  'O',  'M',  '7',  0x00,  // com port
        0x05, 0x00, 0x00, 0x00,        // inputs
        0x03, 0x00, 0x00, 0x00         // inputs
    };

    double* PUser = reinterpret_cast<double*>(data.data());

    WHEN("deserializing the configuration") {
      Configuration configuration(PUser);

      THEN("The configuration was read correctly") {
        REQUIRE(configuration.present);
        REQUIRE(configuration.comPort == "COM7");
        REQUIRE(configuration.inputs == 5);
        REQUIRE(configuration.outputs == 3);
      }
    }
  }

  GIVEN("A configuration") {
    std::array<unsigned char, 1 + 5 + 2 * 4 + 1 + 1> data{
        0x00,                          // present
        0x00, 0x00, 0x00, 0x00, 0x00,  // com port
        0x00, 0x00, 0x00, 0x00,        // inputs
        0x00, 0x00, 0x00, 0x00,        // inputs
        0x00,                          // include input names
        0x00                           // include output names
    };
    double* PUser = reinterpret_cast<double*>(data.data());

    Configuration configuration(PUser);
    configuration.present = true;
    configuration.comPort = "COM1";
    configuration.inputs = 10;
    configuration.outputs = 7;
    configuration.includeInputNames = true;
    configuration.includeOutputNames = true;

    WHEN("serializing the configuration") {
      std::array<unsigned char, 1 + 5 + 2 * 4 + 1 + 1> expectedData{
          0x01,                          // present
          'C',  'O',  'M',  '1',  0x00,  // com port
          0x0A, 0x00, 0x00, 0x00,        // inputs
          0x07, 0x00, 0x00, 0x00,        // inputs
          0x01,                          // include input names
          0x01,                          // include output names
      };

      REQUIRE(configuration.Write());

      THEN("The serialized configuration matches") {
        REQUIRE(data == expectedData);
      }
    }
  }

  GIVEN("A too large configuration") {
    size_t maxSize = sizeof(double) * 100;
    std::array<unsigned char, 1> data{
        0x00,  // present
    };
    double* PUser = reinterpret_cast<double*>(data.data());

    Configuration configuration(PUser);
    configuration.present = true;
    // generate COM name which is one character too large
    configuration.comPort =
        std::string(maxSize, '.');
    configuration.inputs = 10;
    configuration.outputs = 7;

    WHEN("serializing the configuration") {
      THEN("It fails with an error") { REQUIRE_FALSE(configuration.Write()); }
    }
  }
}
