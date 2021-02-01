#pragma once

#include "libusb++/libusb++.hpp"
#include <algorithm>
#include <optional>
#include <string_view>

namespace usb {
namespace utils {
[[nodiscard]] std::vector<device>
filtered_devices(uint16_t vendor_id, uint16_t product_id, context &ctx);

void print_short(const usb::device &d);
void print_recursive(const device &d);
void print(const libusb_device_descriptor &dd, const std::string_view prefix,
           const std::optional<device_handle> &handle = std::nullopt);
void print(const config_descriptor &c, const std::string_view prefix,
           const std::optional<device_handle> &handle = std::nullopt);
void print(const interface_descriptor &i, const std::string_view prefix,
           const std::optional<device_handle> &handle = std::nullopt);
void print(const endpoint_descriptor &e, const std::string_view prefix);
} // namespace utils
} // namespace usb