cmake_minimum_required(VERSION 3.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(Extron-Matrix)

set(SOURCES
	src/configurationdialog.cpp
	src/configurationdialog.h
	src/configuration.cpp
	src/configuration.h
	src/dll.cpp
	src/device.cpp
	src/device.h
	src/listserialports.cpp
	src/listserialports.h
	src/simulation.cpp
	src/simulation.h
)

add_library(${PROJECT_NAME} SHARED
	${SOURCES}
	res/Extron-Matrix.rc
	
	# ProfiLab expects functions with the stdcall calling convention but the names must be unmangled.
	# This can only be achieved by changing the function names with a module definition (.def) file.
	src/dll.def
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/res
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

# Still targeting Windows XP.
target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0502)

# find_package doesn't work with header-only libraries like ASIO. Manually specify dependencies that need to be linked against.
find_package(Boost COMPONENTS system REQUIRED)
target_link_libraries(${PROJECT_NAME} Boost::system)

add_subdirectory(tests)
