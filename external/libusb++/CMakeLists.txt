add_library(libusb++ "")
target_sources(libusb++
	PUBLIC
		include/libusb++/libusb++.hpp
		include/libusb++/utils.hpp
		src/libusb++.cpp
		src/utils.cpp
 "include/libusb++/error.hpp" "include/libusb++/helper.hpp" "include/libusb++/descriptors.hpp")
add_library(libusb++::libusb++ ALIAS libusb++)
target_include_directories(libusb++ PUBLIC include)
target_link_libraries(libusb++ PUBLIC libusb::usb-1.0 fmt::fmt Microsoft.GSL::GSL)