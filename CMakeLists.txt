cmake_minimum_required (VERSION 3.0)
include("cmake/HunterGate.cmake")
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.19.87.tar.gz"
        SHA1 "1b0041a06357d2c9f1f1a7e93ff3132c36411719"
)

project (CMakeHelloWorld)

hunter_add_package(cereal)
find_package(cereal CONFIG REQUIRED)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
#version number
set (CMakeHelloWorld_VERSION_MAJOR 1)
set (CMakeHelloWorld_VERSION_MINOR 0)

#include the subdirectory containing our libs
include_directories(Hello
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CEREAL_ROOT}/include)
message("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_subdirectory (Hello)
add_subdirectory(src)
#indicate the entry point for the executable
add_executable (CMakeHelloWorld Hello HelloWorld.cpp)
# Indicate which libraries to include during the link process.
target_link_libraries (CMakeHelloWorld Hello scp )

install (TARGETS CMakeHelloWorld DESTINATION bin)
