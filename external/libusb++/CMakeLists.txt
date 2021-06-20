add_library(libusb++ "")
target_sources(libusb++
    PRIVATE
        src/libusb++.cpp
        src/utils.cpp
        include/libusb++/libusb++.hpp
        include/libusb++/utils.hpp
        include/libusb++/error.hpp
        include/libusb++/helper.hpp
        include/libusb++/descriptors.hpp
        include/libusb++/logging.hpp
 "include/libusb++/device_filter.hpp" "src/device_filter.cpp")
add_library(libusb++::libusb++ ALIAS libusb++)
target_include_directories(libusb++ PUBLIC include)
target_link_libraries(libusb++
    PUBLIC
        fmt::fmt
        Microsoft.GSL::GSL
        spdlog::spdlog)

# on windows use self-built libusb, on linux use system one
if(WIN32)
	target_link_libraries(libusb++ PUBLIC libusb::usb-1.0)
elseif(UNIX)
	target_link_libraries(libusb++ PUBLIC usb-1.0)
endif()