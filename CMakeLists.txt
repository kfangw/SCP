cmake_minimum_required(VERSION 3.0)
include("cmake/HunterGate.cmake")
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.19.87.tar.gz"
        SHA1 "1b0041a06357d2c9f1f1a7e93ff3132c36411719"
)

project(SCP)
set(SCP_VERSION_MAJOR 1)
set(SCP_VERSION_MINOR 0)

hunter_add_package(cereal)
find_package(cereal CONFIG REQUIRED)
hunter_add_package(OpenSSL)
#find_package(OpenSSL CONFIG REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#include the subdirectory containing our libs
include_directories(Hello
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CEREAL_ROOT}/include
        ${OpenSSL_ROOT}/include
        )

message("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(src)

