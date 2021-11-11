set(CMAKE_C_CLANG_TIDY "")
set(CMAKE_CXX_CLANG_TIDY "")

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

if(WIN32)
    include("libusb.CMakeLists.cmake")
endif()

CPMAddPackage(
    NAME fmt
    GIT_TAG 7.1.3
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)

CPMAddPackage(
    NAME spdlog
    GIT_TAG v1.8.2
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    OPTIONS "SPDLOG_FMT_EXTERNAL ON"
)

CPMAddPackage(
    NAME GSL
    GIT_TAG 84aeb59f269ee9747e85d780c8aa180bca3a9a60 # shortly after v3.1.0
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
)

if(spinaltap_BUILD_TOOLS)
    CPMAddPackage(
        NAME clipp
        GIT_TAG 7addf48c58ac8eb900cc3335f86e78ffae20735d # c++20, but fetchcontent doesn't like branch names with +
        GIT_REPOSITORY https://github.com/andreasWallner/clipp.git
    )
endif()

if(spinaltap_BUILD_TESTS)
    CPMAddPackage(
        Catch2
        GIT_TAG 3f8cae8025f4f5e804383f44bb393a63bcef90a4
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    )
endif()

add_subdirectory(libusb++ EXCLUDE_FROM_ALL)
add_subdirectory(spinaltap EXCLUDE_FROM_ALL)
