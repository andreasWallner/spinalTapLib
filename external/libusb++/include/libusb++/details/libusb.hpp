#ifndef libusbxx_details_libusb_hpp
#define libusbxx_details_libusb_hpp

// since libusb includes windows.h, try to make impact
// as small as possible
// TODO look for a workaround to not have to include
// windows.h indirectly or libusb
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
// either include from libusb cmake build or use the
// system available libusb installation
#if __has_include("libusb.h")
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#undef NOMINMAX
#undef NOCOMM
#undef WIN32_LEAN_AND_MEAN

#endif
