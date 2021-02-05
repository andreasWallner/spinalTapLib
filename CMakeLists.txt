cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ztex-if C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_STANDARD 99)

if(MSVC_VERSION GREATER 1910) # 1911 is 15.3 update
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -permissive-")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -permissive-")
endif()

#set(WITH_DEBUG_LOG YES)
#set(WITH_LOGGING YES)
add_subdirectory(external)

add_executable(test_libusbpp "src/test_usb++.cpp" "src/ztexpp.cpp" "src/utils.cpp" "src/spinaltap.hpp" "src/spinaltap.cpp")
target_link_libraries(test_libusbpp libztex fmt::fmt clipp::clipp Microsoft.GSL::GSL libusb++::libusb++)

add_executable(torturetest
    "src/torturetest.main.cpp"
    "src/utils.cpp"
)
target_link_libraries(torturetest fmt::fmt clipp::clipp Microsoft.GSL::GSL libusb++::libusb++)

add_executable(test "src/test.cpp")
target_link_libraries(test Catch2::Catch2WithMain)