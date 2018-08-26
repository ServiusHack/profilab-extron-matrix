add_executable(${PROJECT_NAME}_Unittests
	${CMAKE_SOURCE_DIR}/src/configuration.cpp
	${CMAKE_SOURCE_DIR}/src/configuration.h
	configuration_test.cpp
	${CMAKE_SOURCE_DIR}/src/listserialports.cpp
	${CMAKE_SOURCE_DIR}/src/listserialports.h
	listserialports_test.cpp
)

find_path(CATCH_INCLUDE_DIR catch.hpp)
target_include_directories(${PROJECT_NAME}_Unittests SYSTEM PRIVATE ${CATCH_INCLUDE_DIR})

target_include_directories(${PROJECT_NAME}_Unittests SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(Unittests ${PROJECT_NAME}_Unittests)