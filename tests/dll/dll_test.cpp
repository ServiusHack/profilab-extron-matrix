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

  GIVEN("A configuration with 5 inputs and 2 outputs") {
    std::unique_ptr<double> PUser(new double);
    ALLOW_CALL(configurationMockInstance, Constructor(PUser.get(), _))
        .LR_SIDE_EFFECT(_2.present = true)
        .LR_SIDE_EFFECT(_2.comPort = "COM1")
        .LR_SIDE_EFFECT(_2.inputs = 5)
        .LR_SIDE_EFFECT(_2.outputs = 2);

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
        .LR_SIDE_EFFECT(_2.outputs = 2);

    Device* device;
    ALLOW_CALL(deviceMockInstance, Constructor(_, 5, 2, _))
        .LR_SIDE_EFFECT(device = _1);

    ALLOW_CALL(deviceMockInstance, open("COM1"));

    CSimStart(PInput.data(), POutput.data(), PUser.data());

    WHEN("the first pin is set to 5.0") {
      PInput[0] = 5.0;

      THEN("store to the 5th preset on the device") {
        REQUIRE_CALL(deviceMockInstance, store(5));

        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        WHEN("the first pin is set to 4.999") {
          PInput[0] = 4.999;

          THEN("store to the 4th preset on the device") {
            REQUIRE_CALL(deviceMockInstance, store(4));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
        }

        WHEN("the first pin is set to 5.999") {
          PInput[0] = 5.999;

          THEN("nothing happens") {
            FORBID_CALL(deviceMockInstance, store(_));

            CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                         PStrings.data());
          }
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

    WHEN("the second pin is set to 3.0") {
      PInput[1] = 3.0;

      THEN("the 3rd preset is recalled on the device") {
        REQUIRE_CALL(deviceMockInstance, recall(3));

        CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                     PStrings.data());

        WHEN("the second pin is set to 2.999") {
          PInput[1] = 2.999;

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

    WHEN("connected to device") {
      device->connectedCallback();
      CCalculateEx(PInput.data(), POutput.data(), PUser.data(),
                   PStrings.data());

      THEN("the first pin is set to 5.0") { REQUIRE(POutput[0] == 5.0); }
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
