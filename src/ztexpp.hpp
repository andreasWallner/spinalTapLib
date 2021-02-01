#pragma once

#include "libusb++/libusb++.hpp"

#include <string>
#include <filesystem>

namespace ztex {

struct dev_info {
  uint8_t fx_version;
  uint8_t board_series;
  uint8_t board_number;
  std::string board_variant;
  uint8_t fast_config_ep;
  uint8_t fast_config_if;
  uint8_t default_version1;
  uint8_t default_version2;
  uint8_t default_out_ep;
  uint8_t default_in_ep;
};

[[nodiscard]] std::string device_info_string(const usb::device &dev);
[[nodiscard]] dev_info device_info(usb::interface &intf);
[[nodiscard]] bool is_fpga_configured(usb::interface &intf);
void upload_bitstream(usb::interface &intf, dev_info &info,
                      std::filesystem::path bitstream_location);
uint8_t ctrl_gpio(usb::interface &intf, uint8_t mask, uint8_t value);
void reset_fpga(usb::interface &intf, bool leave);

} // namespace ztex