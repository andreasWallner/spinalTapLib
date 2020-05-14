cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ztex-if C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_STANDARD 99)

add_subdirectory(external/libusb-1.0.23/libusb)
add_subdirectory(external/libztex)

add_executable(memfifo src/memfifo.cpp)
target_link_libraries(memfifo libztex)