#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <array>

#include "trompeloeil.hpp"

#include "configuration_mock.h"
#include "configurationdialog_mock.h"
#include "device_mock.h"

namespace trompeloeil {
template <>
void reporter<specialized>::send(severity s,
                                 const char* file,
                                 unsigned long line,
                                 const char* msg) {
  std::ostringstream os;
  if (line)
    os << file << ':' << line << '\n';
  os << msg;
  auto failure = os.str();
  if (s == severity::fatal) {
    FAIL(failure);
  } else {
    CAPTURE(failure);
    CHECK(failure.empty());
  }
}
}  // namespace trompeloeil

using trompeloeil::_;

#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT void __stdcall CConfigure(double* PUser);
DLLEXPORT unsigned char __stdcall CNumInputsEx(double* PUser);
DLLEXPORT unsigned char __stdcall CNumOutputsEx(double* PUser);
DLLEXPORT void __stdcall GetInputName(unsigned char Channel,
                                      unsigned char* Name);
DLLEXPORT void __stdcall GetOutputName(unsigned char Channel,
                                       unsigned char* Name);
DLLEXPORT void __stdcall CSimStart(double* /*PInput*/,
                                   double* /*POutput*/,
                                   double* PUser);
DLLEXPORT void __stdcall CCalculateEx(double* PInput,
                                      double* POutput,
                                      double* /*PUser*/,
                                      char** PStrings);
DLLEXPORT void __stdcall CSimStop(double* /*PInput*/,
                                  double* /*POutput*/,
                                  double* /*PUser*/);

SCENARIO("Configuration the DLL", "[dll]") {
  WHEN("Calling CConfigure") {
    std::unique_ptr<double> PUser(new double);
    REQUIRE_CALL(configurationDialogMockInstance, Get());
    REQUIRE_CALL(configurationMockInstance, Write()).RETURN(true);
    CConfigure(PUser.get());
  }
}

SCENARIO("Input and output pins", "[dll]") {
  GIVEN("No configuration") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = false);

    WHEN("Calling CNumInputsEx") {
      unsigned char inputs = CNumInputsEx(PUser.get());
      THEN("0 is returned") { REQUIRE(inputs == 0); }
    }
    WHEN("Calling CNumOutputsEx") {
      unsigned char outputs = CNumOutputsEx(PUser.get());
      THEN("0 is returned") { REQUIRE(outputs == 0); }
    }
  }

  GIVEN("A configuration with 5 inputs and 2 outputs without names") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2)
        .LR_SIDE_EFFECT(_2.includeInputNames = false)
        .LR_SIDE_EFFECT(_2.includeOutputNames = false);

    WHEN("Calling CNumInputsEx") {
      unsigned char inputs = CNumInputsEx(PUser.get());
      THEN("4 inputs are returned") { REQUIRE(inputs == 4); }
    }

    WHEN("Getting first input name") {
      std::array<unsigned char, 100> name;
      GetInputName(0, name.data());
      THEN("It is 'STORE'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "STORE");
      }
    }

    WHEN("Getting second input name") {
      std::array<unsigned char, 100> name;
      GetInputName(1, name.data());
      THEN("It is 'RECALL'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "RECALL");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(2, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(3, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Calling CNumOutputsEx") {
      unsigned char outputs = CNumOutputsEx(PUser.get());
      THEN("5 outputs are returned") { REQUIRE(outputs == 5); }
    }

    WHEN("Getting first output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(0, name.data());
      THEN("It is 'CON'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "CON");
      }
    }

    WHEN("Getting second output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(1, name.data());
      THEN("It is 'ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "ERR");
      }
    }

    WHEN("Getting third output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(2, name.data());
      THEN("It is '$ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$ERR");
      }
    }

    WHEN("Getting fourth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(3, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting fifth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(4, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }
  }

  GIVEN("A configuration with 5 inputs and 2 outputs with input names") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2)
        .LR_SIDE_EFFECT(_2.includeInputNames = true)
        .LR_SIDE_EFFECT(_2.includeOutputNames = false);

    WHEN("Calling CNumInputsEx") {
      unsigned char inputs = CNumInputsEx(PUser.get());
      THEN("9 inputs are returned") { REQUIRE(inputs == 9); }
    }

    WHEN("Getting first input name") {
      std::array<unsigned char, 100> name;
      GetInputName(0, name.data());
      THEN("It is 'STORE'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "STORE");
      }
    }

    WHEN("Getting second input name") {
      std::array<unsigned char, 100> name;
      GetInputName(1, name.data());
      THEN("It is 'RECALL'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "RECALL");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(2, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting 4th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(3, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting 5th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(4, name.data());
      THEN("It is '$I0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I0");
      }
    }

    WHEN("Getting 6th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(5, name.data());
      THEN("It is '$I1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I1");
      }
    }

    WHEN("Getting 7th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(6, name.data());
      THEN("It is '$I2'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I2");
      }
    }

    WHEN("Getting 8th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(7, name.data());
      THEN("It is '$I3'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I3");
      }
    }

    WHEN("Getting 9th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(8, name.data());
      THEN("It is '$I4'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I4");
      }
    }

    WHEN("Calling CNumOutputsEx") {
      unsigned char outputs = CNumOutputsEx(PUser.get());
      THEN("10 outputs are returned") { REQUIRE(outputs == 10); }
    }

    WHEN("Getting first output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(0, name.data());
      THEN("It is 'CON'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "CON");
      }
    }

    WHEN("Getting second output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(1, name.data());
      THEN("It is 'ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "ERR");
      }
    }

    WHEN("Getting third output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(2, name.data());
      THEN("It is '$ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$ERR");
      }
    }

    WHEN("Getting 4th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(3, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting 5th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(4, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting 6th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(5, name.data());
      THEN("It is '$I0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I0");
      }
    }

    WHEN("Getting 7th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(6, name.data());
      THEN("It is '$I1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I1");
      }
    }

    WHEN("Getting 8th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(7, name.data());
      THEN("It is '$I2'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I2");
      }
    }

    WHEN("Getting 9th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(8, name.data());
      THEN("It is '$I3'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I3");
      }
    }

    WHEN("Getting 10th output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(9, name.data());
      THEN("It is '$I4'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I4");
      }
    }
  }

  GIVEN("A configuration with 5 inputs and 2 outputs with output names") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2)
        .LR_SIDE_EFFECT(_2.includeInputNames = false)
        .LR_SIDE_EFFECT(_2.includeOutputNames = true);

    WHEN("Calling CNumInputsEx") {
      unsigned char inputs = CNumInputsEx(PUser.get());
      THEN("6 inputs are returned") { REQUIRE(inputs == 6); }
    }

    WHEN("Getting first input name") {
      std::array<unsigned char, 100> name;
      GetInputName(0, name.data());
      THEN("It is 'STORE'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "STORE");
      }
    }

    WHEN("Getting second input name") {
      std::array<unsigned char, 100> name;
      GetInputName(1, name.data());
      THEN("It is 'RECALL'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "RECALL");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(2, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting fourth input name") {
      std::array<unsigned char, 100> name;
      GetInputName(3, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting 5th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(4, name.data());
      THEN("It is '$O0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O0");
      }
    }

    WHEN("Getting 6th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(5, name.data());
      THEN("It is '$O1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O1");
      }
    }

    WHEN("Calling CNumOutputsEx") {
      unsigned char outputs = CNumOutputsEx(PUser.get());
      THEN("7 outputs are returned") { REQUIRE(outputs == 7); }
    }

    WHEN("Getting first output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(0, name.data());
      THEN("It is 'CON'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "CON");
      }
    }

    WHEN("Getting second output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(1, name.data());
      THEN("It is 'ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "ERR");
      }
    }

    WHEN("Getting third output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(2, name.data());
      THEN("It is '$ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$ERR");
      }
    }

    WHEN("Getting fourth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(3, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting fifth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(4, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting sixth input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(5, name.data());
      THEN("It is '$O0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O0");
      }
    }

    WHEN("Getting seventh input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(6, name.data());
      THEN("It is '$O1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O1");
      }
    }
  }

  GIVEN("A configuration with 5 inputs and 2 outputs with names") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2)
        .LR_SIDE_EFFECT(_2.includeInputNames = true)
        .LR_SIDE_EFFECT(_2.includeOutputNames = true);

    WHEN("Calling CNumInputsEx") {
      unsigned char inputs = CNumInputsEx(PUser.get());
      THEN("11 inputs are returned") { REQUIRE(inputs == 11); }
    }

    WHEN("Getting first input name") {
      std::array<unsigned char, 100> name;
      GetInputName(0, name.data());
      THEN("It is 'STORE'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "STORE");
      }
    }

    WHEN("Getting second input name") {
      std::array<unsigned char, 100> name;
      GetInputName(1, name.data());
      THEN("It is 'RECALL'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "RECALL");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(2, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting third input name") {
      std::array<unsigned char, 100> name;
      GetInputName(3, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting 5th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(4, name.data());
      THEN("It is '$I0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I0");
      }
    }

    WHEN("Getting 6th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(5, name.data());
      THEN("It is '$I1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I1");
      }
    }

    WHEN("Getting 7th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(6, name.data());
      THEN("It is '$I2'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I2");
      }
    }

    WHEN("Getting 8th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(7, name.data());
      THEN("It is '$I3'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I3");
      }
    }

    WHEN("Getting 9th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(8, name.data());
      THEN("It is '$I4'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I4");
      }
    }

    WHEN("Getting 10th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(9, name.data());
      THEN("It is '$O0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O0");
      }
    }

    WHEN("Getting 11th input name") {
      std::array<unsigned char, 100> name;
      GetInputName(10, name.data());
      THEN("It is '$O1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O1");
      }
    }

    WHEN("Calling CNumOutputsEx") {
      unsigned char outputs = CNumOutputsEx(PUser.get());
      THEN("12 outputs are returned") { REQUIRE(outputs == 12); }
    }

    WHEN("Getting first output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(0, name.data());
      THEN("It is 'CON'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "CON");
      }
    }

    WHEN("Getting second output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(1, name.data());
      THEN("It is 'ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "ERR");
      }
    }

    WHEN("Getting third output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(2, name.data());
      THEN("It is '$ERR'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$ERR");
      }
    }

    WHEN("Getting fourth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(3, name.data());
      THEN("It is 'OUT0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT0");
      }
    }

    WHEN("Getting fifth output name") {
      std::array<unsigned char, 100> name;
      GetOutputName(4, name.data());
      THEN("It is 'OUT1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "OUT1");
      }
    }

    WHEN("Getting 6th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(5, name.data());
      THEN("It is '$I0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I0");
      }
    }

    WHEN("Getting 7th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(6, name.data());
      THEN("It is '$I1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I1");
      }
    }

    WHEN("Getting 8th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(7, name.data());
      THEN("It is '$I2'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I2");
      }
    }

    WHEN("Getting 9th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(8, name.data());
      THEN("It is '$I3'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I3");
      }
    }

    WHEN("Getting 10th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(9, name.data());
      THEN("It is '$I4'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$I4");
      }
    }

    WHEN("Getting 11th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(10, name.data());
      THEN("It is '$O0'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O0");
      }
    }

    WHEN("Getting 12th input name") {
      std::array<unsigned char, 100> name;
      GetOutputName(11, name.data());
      THEN("It is '$O1'") {
        std::string str(reinterpret_cast<char*>(&name.front()));
        CHECK(str == "$O1");
      }
    }
  }
}

SCENARIO("Simulation", "[dll]") {
  std::array<double, 100> PInput{};
  std::array<double, 100> POutput{};
  std::array<std::array<char, 1000>, 100> PStringsMemory{};
  std::array<char*, 100> PStrings;
  for (size_t i = 0; i < PStringsMemory.size(); ++i) {
    PStrings[i] = PStringsMemory[i].data();
  }
  std::array<double, 100> PUser{};

  WHEN("running the simulation") {
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.data(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2)
        .LR_SIDE_EFFECT(_2.includeInputNames = true)
        .LR_SIDE_EFFECT(_2.includeOutputNames = true);

    Device* device;
    ALLOW_CALL(deviceMockInstance, Constructor(_, _))
        .LR_SIDE_EFFECT(device = _1);

    ALLOW_CALL(deviceMockInstance, open("COM1"));

    CSimStart(PInput.data(), POutput.data(), PUser.data());

    WHEN("the first pin is set to 5.0") {
      PInput[0] = 5.0;

      THEN("store to the 5th preset on the device") {
        REQUIRE_CALL(deviceMockInstance, store(5));

        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        WHEN("the first pin is set to 5.5") {
          PInput[0] = 5.5;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, store(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the first pin is set to 4.3") {
          PInput[0] = 4.3;

          THEN("store to the 4th preset on the device") {
            REQUIRE_CALL(deviceMockInstance, store(4));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the first pin is set to 0.0") {
          PInput[0] = 0.0;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, store(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }
      }

    }

    WHEN("the second pin is set to 3.0") {
      PInput[1] = 3.0;

      THEN("the 3rd preset is recalled on the device") {
        REQUIRE_CALL(deviceMockInstance, recall(3));

        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        WHEN("the second pin is set to 3.5") {
          PInput[1] = 3.5;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, recall(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the second pin is set to 3.999") {
          PInput[1] = 3.999;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, recall(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the second pin is set to 2.3") {
          PInput[1] = 2.3;

          THEN("the 2nd preset is recalled on the device") {
            REQUIRE_CALL(deviceMockInstance, recall(2));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the second pin is set to 3.999") {
          PInput[1] = 3.999;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, recall(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }
      }

      WHEN("the second pin is set to 2.999") {
        PInput[1] = 2.999;

        THEN("the 2nd preset is recalled on the device") {
          REQUIRE_CALL(deviceMockInstance, recall(2));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
        }
      }

      WHEN("the second pin is set to 0.0") {
        PInput[1] = 0.0;

        THEN("nothing happens") {
          FORBID_CALL(deviceMockInstance, recall(_));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
        }
      }
    }

    WHEN("the third pin is set to 2.0") {
      PInput[2] = 2.0;

      THEN("the input 2 is tied to the output 1") {
        REQUIRE_CALL(deviceMockInstance, tie(2, 1));

        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        WHEN("the third pin is set to 2.8") {
          PInput[2] = 2.8;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, tie(_, _));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the third pin is set to 1.1") {
          PInput[2] = 1.1;

          THEN("the input 1 is tied to the output 1") {
            REQUIRE_CALL(deviceMockInstance, tie(1, 1));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the third pin is set to 4.0") {
          PInput[2] = 4.0;

          THEN("the input 4 is tied to the output 1") {
            REQUIRE_CALL(deviceMockInstance, tie(4, 1));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the third pin is set to 0.0") {
          PInput[2] = 0;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, tie(_, _));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }
      }
    }

    WHEN("the fifth pin is set to 'abc'") {
      memcpy(PStrings[4], "abc", 4);

      THEN("the first input name is set to 'abc'") {
        REQUIRE_CALL(deviceMockInstance, set_input_name(1, "abc"));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
      }
    }

    WHEN("the nineth pin is set to 'xyz'") {
      memcpy(PStrings[8], "xyz", 4);

      THEN("the 5th input name is set to 'xyz'") {
        REQUIRE_CALL(deviceMockInstance, set_input_name(5, "xyz"));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
      }
    }

    WHEN("the tenth pin is set to 'def'") {
      memcpy(PStrings[9], "def", 4);

      THEN("the first output name is set to 'def'") {
        REQUIRE_CALL(deviceMockInstance, set_output_name(1, "def"));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
      }
    }

    WHEN("the eleventh pin is set to '123'") {
      memcpy(PStrings[10], "123", 4);

      THEN("the second output name is set to '123'") {
        REQUIRE_CALL(deviceMockInstance, set_output_name(2, "123"));

          CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                       PStrings.data());
      }
    }

    WHEN("connected to device") {
      device->connectedCallback();
      CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                   PStrings.data());

      THEN("the first pin is set to 5.0") { REQUIRE(POutput[0] == 5.0); }

      WHEN("device has different number of outputs") {
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_outputs()).TIMES(AT_LEAST(1)).RETURN(1);
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_inputs()).TIMES(AT_LEAST(1)).RETURN(5);
        device->setupCallback();
        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        THEN("the second pin is set to 5.0") {
          REQUIRE(POutput[1] == 5.0);
        }

        THEN("the third pin is set to the error message") {
          REQUIRE(std::string(PStrings[2]) == "Device has 1 outputs but DLL is configured for 2.");
        }
      }

      WHEN("device has different number of inputs") {
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_outputs()).TIMES(AT_LEAST(1)).RETURN(2);
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_inputs()).TIMES(AT_LEAST(1)).RETURN(4);
        device->setupCallback();
        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        THEN("the second pin is set to 5.0") {
          REQUIRE(POutput[1] == 5.0);
        }

        THEN("the third pin is set to the error message") {
          REQUIRE(std::string(PStrings[2]) == "Device has 4 inputs but DLL is configured for 5.");
        }
      }

      WHEN("device has different number of outputs and inputs") {
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_outputs()).TIMES(AT_LEAST(1)).RETURN(3);
        REQUIRE_CALL(deviceMockInstance, get_number_of_virtual_inputs()).TIMES(AT_LEAST(1)).RETURN(4);
        device->setupCallback();
        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        THEN("the second pin is set to 5.0") {
          REQUIRE(POutput[1] == 5.0);
        }

        THEN("the third pin is set to an error message") {
          REQUIRE(!std::string(PStrings[2]).empty());
        }
      }
    }

    WHEN("a device error was encountered") {
      const std::string errorMessage("test error message");
      device->reportError(errorMessage);
      CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                   PStrings.data());

      THEN("the second pin is set to 5.0") { REQUIRE(POutput[1] == 5.0); }

      THEN("the third pin is set to the error message") {
        REQUIRE(std::string(PStrings[2]) == errorMessage);
      }
    }

    WHEN("input 4 is tied to output 2 on the device") {
      device->tieChanged(2, 4);
      CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                   PStrings.data());

      THEN("the 5th pin is set to 4.0") { REQUIRE(POutput[4] == 4.0); }
    }

    ALLOW_CALL(deviceMockInstance, close());

    CSimStop(PInput.data(), POutput.data(), PUser.data());
  }
}
