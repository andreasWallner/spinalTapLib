set(CMAKE_C_CLANG_TIDY "")
set(CMAKE_CXX_CLANG_TIDY "")

if(WIN32)
    include("libusb.CMakeLists.cmake")
endif()

#add_subdirectory(libztex)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(clipp EXCLUDE_FROM_ALL)
add_subdirectory(GSL EXCLUDE_FROM_ALL)
add_subdirectory(Catch2 EXCLUDE_FROM_ALL)
add_subdirectory(libusb++ EXCLUDE_FROM_ALL)
