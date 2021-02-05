add_library(libztex ztex.c)
target_include_directories(libztex PUBLIC .)
target_link_libraries(libztex PUBLIC libusb::usb-1.0)