#pragma once

#include "libusb++/libusb++.hpp"
#include <optional>
#include <vector>

namespace usb {

/// Check a device against a set of predicates
class device_filter {
private:
  std::optional<int> bus_;
  std::optional<int> device_address_;
  std::optional<uint16_t> vendor_id_;
  std::optional<uint16_t> product_id_;
  std::optional<uint8_t> interface_class_;
  std::optional<uint8_t> interface_subclass_;
  std::optional<uint8_t> interface_protocol_;
  // serial number?
public:
  void bus(int expected);
  void device_address(int expected);
  void vendor_id(uint16_t vid);
  void product_id(uint16_t pid);
  void interface_class(uint8_t expected);
  void interface_subclass(uint8_t expected);
  void interface_protocol(uint8_t expected);

  [[nodiscard]] bool test(const usb::device &d) const;
  [[nodiscard]] std::vector<device> filter(const device_list &l) const;
};

} // namespace usb
