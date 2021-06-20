#pragma once

#include "spdlog/spdlog.h"

#include "spdlog/fmt/bin_to_hex.h"

namespace usb::logging {

inline auto logger = std::make_shared<spdlog::logger>("libusb++");

} // namespace usb::logging
