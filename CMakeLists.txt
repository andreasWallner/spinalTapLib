cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ztex-if C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_STANDARD 99)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    set(spinaltap_ROOT_PROJECT ON)
else()
    set(spinaltap_ROOT_PROJECT OFF)
endif()

option(spinaltap_BUILD_TESTS "Build unittests for spinaltap" ${spinaltap_ROOT_PROJECT})
option(spinaltap_BUILD_TOOLS "Build spinaltap tools" ${spinaltap_ROOT_PROJECT})

include(cmake/CPM.cmake)

if(spinaltap_ROOT_PROJECT)
    if(MSVC_VERSION GREATER 1910) # 1911 is 15.3 update
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -permissive-")
        set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -permissive-")
    endif()
endif()

find_package(Threads)

add_subdirectory(external)

if(spinaltap_BUILD_TOOLS)
    add_executable(test_libusbpp "src/test_usb++.cpp" "src/ztexpp.cpp" "src/utils.cpp")
    target_link_libraries(test_libusbpp
        fmt::fmt
        clipp::clipp
        Microsoft.GSL::GSL
        libusb++::libusb++
        spdlog::spdlog
        spinaltap::spinaltap
    )

    add_executable(torturetest
        "src/torturetest.main.cpp"
        "src/utils.cpp"
    )
    target_link_libraries(torturetest fmt::fmt clipp::clipp Microsoft.GSL::GSL libusb++::libusb++ Threads::Threads)
endif()

if(spinaltap_BUILD_TESTS)
    add_executable(test "src/test.cpp")
    target_link_libraries(test Catch2::Catch2WithMain)
endif()
