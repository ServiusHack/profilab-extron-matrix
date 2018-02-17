add_executable(${PROJECT_NAME}_DLLTests
 ${CMAKE_SOURCE_DIR}/src/dll.cpp # SUT
 ${CMAKE_SOURCE_DIR}/src/simulation.cpp
 dll_test.cpp # Tests
 ${CMAKE_SOURCE_DIR}/tests/mocks/configuration_mock.cpp
 ${CMAKE_SOURCE_DIR}/tests/mocks/configurationdialog_mock.cpp
 ${CMAKE_SOURCE_DIR}/tests/mocks/device_mock.cpp
)
add_test(DLL ${PROJECT_NAME}_DLLTests)

target_include_directories(${PROJECT_NAME}_DLLTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_include_directories(${PROJECT_NAME}_DLLTests PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_include_directories(${PROJECT_NAME}_DLLTests PRIVATE ${CMAKE_SOURCE_DIR}/tests/mocks)

target_include_directories(${PROJECT_NAME}_DLLTests SYSTEM PRIVATE ${CATCH_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}_DLLTests Boost::system)

target_compile_definitions(${PROJECT_NAME}_DLLTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

# Still targeting Windows XP.
target_compile_definitions(${PROJECT_NAME}_DLLTests PRIVATE _WIN32_WINNT=0x0502)