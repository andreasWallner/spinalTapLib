#pragma once

#include "libusb++/libusb++.hpp"
#include <filesystem>

namespace spinaltap {
namespace control {
void load_bitstream(const usb::interface &intf,
                    const std::filesystem::path &location);
}
} // namespace spinaltap