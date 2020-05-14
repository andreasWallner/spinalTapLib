add_library(libztex ztex.c)
target_include_directories(libztex PUBLIC .)
target_link_libraries(libztex PUBLIC usb-1.0-static)