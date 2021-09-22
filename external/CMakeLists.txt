set(CMAKE_C_CLANG_TIDY "")
set(CMAKE_CXX_CLANG_TIDY "")

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

if(WIN32)
    include("libusb.CMakeLists.cmake")
endif()

include(FetchContent)
FetchContent_Declare(
    FMT
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 7.1.3
)
FetchContent_MakeAvailable(FMT)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.8.2
)
# since CMP0077 we would be able to not have that as a cache variable,
# but since spdlog only requires 3.10 we can't yet do so 
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library instead of bundled")
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    GSL
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG 84aeb59f269ee9747e85d780c8aa180bca3a9a60 # shortly after v3.1.0
)
FetchContent_MakeAvailable(GSL)

if(spinaltap_BUILD_TOOLS)
    FetchContent_Declare(
        clipp
        GIT_REPOSITORY https://github.com/andreasWallner/clipp.git
        GIT_TAG 7addf48c58ac8eb900cc3335f86e78ffae20735d # c++20, but fetchcontent doesn't like branch names with +
    )
    FetchContent_MakeAvailable(clipp)
endif()

if(spinaltap_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG 3f8cae8025f4f5e804383f44bb393a63bcef90a4
    )
    FetchContent_MakeAvailable(Catch2)
endif()

add_subdirectory(libusb++ EXCLUDE_FROM_ALL)
add_subdirectory(spinaltap EXCLUDE_FROM_ALL)
