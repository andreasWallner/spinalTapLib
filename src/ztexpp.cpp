#include "ztexpp.hpp"

#include "fmt/core.h"
#include "gsl/span"
#include "libusb++/libusb++.hpp"
#include "numeric_utils.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace ztex {

class ztex_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

enum class commands : uint8_t {
  usb3_errors = 0x29,
  flash_info = 0x40,
  flash_read = 0x41,
  flash_write = 0x42,
  flash_info2 = 0x43,

  mac_eeprom_info = 0x3D,
  mac_eeprom_read = 0x3B,
  mac_eeprom_write = 0x3C,

  reset_toggle = 0x60,
  gpio_ctrl = 0x61,
  lsi_write = 0x62,
  lsi_read = 0x63,
  lsi_info = 0x64,

  flash2_info = 0x44,
  flash2_read = 0x45,
  flash2_write = 0x46,

  fpga_info = 0x30,
  fpga_reset = 0x31,
  fpga_send = 0x32,
  fpga_fast_info = 0x33,
  fpga_fast_start = 0x34,
  fpga_fast_finish = 0x35
};

[[nodiscard]] std::string device_info_string(const usb::device &dev) {
  auto dd{dev.device_descriptor()};
  std::string product_string = "<unknown>";
  std::string serialnumber = "<unknown>";
  try {
    usb::device_handle devhandle{dev};
    product_string = devhandle.string_descriptor_ascii(dd.iProduct);
    serialnumber = devhandle.string_descriptor_ascii(dd.iSerialNumber);
  } catch (const usb::usb_error &) {
  }
  return fmt::format(
      "Bus {:03x} device {:03x}: ID {:04x}:{:04x} Product {} SN {}",
      dev.bus_number(), dev.device_address(), dd.idVendor, dd.idProduct,
      product_string, serialnumber);
}

[[nodiscard]] dev_info device_info(usb::interface &intf) {
  using namespace usb;
  using namespace std::literals::chrono_literals;

  std::vector<uint8_t> buffer(128, 0);

  dev_info info;

  try {
    const auto fast_config_len{intf.control_read(
        type::vendor, recipient::device, 0x33, 0, 0, buffer, 1500ms)};
    info.fast_config_ep = fast_config_len >= 1 ? buffer[0] : 0;
    info.fast_config_if = fast_config_len == 2 ? buffer[1] : 0;
  } catch (const usb::usb_error &) {
    // if device does not have fast config, it will also not
    // respond to command to get info about fast config
    info.fast_config_ep = 0;
    info.fast_config_if = 0;
  }

  const auto config_len{intf.control_read(type::vendor, recipient::device, 0x3b,
                                          0, 0, buffer, 1500ms)};
  if (config_len != 128 || buffer[0] != 'C' || buffer[1] != 'D' ||
      buffer[2] != '0')
    throw ztex_error("invalid response when reading configuration");
  info.fx_version = buffer[3];
  info.board_series = buffer[4];
  info.board_number = buffer[5];
  info.board_variant = std::string{(char)buffer[6], (char)buffer[7]};

  const auto intf_info_len{intf.control_read(type::vendor, recipient::device,
                                             0x64, 0, 0, buffer, 1500ms)};
  if (intf_info_len < 2 || buffer[0] == 0)
    throw ztex_error("invalid response when reading interface configuration");
  info.default_version1 = buffer[0];
  info.default_version2 = intf_info_len > 3 ? buffer[3] : 0;
  info.default_out_ep = buffer[1] & 0x7f;
  info.default_in_ep = buffer[2] | 0x80;

  return info;
}

[[nodiscard]] bool is_fpga_configured(usb::interface &intf) {
  using namespace usb;
  using namespace std::literals::chrono_literals;
  std::vector<uint8_t> buffer(16, 0);
  const auto rx_len{intf.control_read(type::vendor, recipient::device,
                                      static_cast<uint8_t>(commands::fpga_info),
                                      0, 0, buffer, 1500ms)};
  if (rx_len == 0)
    throw ztex_error("invalid response reading FPGA configuration state");
  return buffer[0] == 0;
}

void upload_bitstream(usb::interface &intf, dev_info &info,
                      std::filesystem::path bitstream_location) {
  constexpr int ep0_transaction_size = 2048;
  static_assert(
      ep0_transaction_size % 64 == 0,
      "transaction size must be x * 64, otherwise end-detection will not work");

  std::ifstream file{bitstream_location, std::ios::binary | std::ios::ate};
  if (!file)
    throw std::system_error(errno, std::system_category(),
                            std::string("failed to open ") +
                                bitstream_location.string());
  const std::streamsize filesize{file.tellg()};
  file.seekg(0, std::ios::beg);

  // load bitstream prepended with 512 0-bytes as dummy data
  // sometimes first 512 byte are swallowed by by FX3 on bulk endpoints
  // if last transfer is less than a full transaction but
  // of a multiple of 64 - add a single byte for end detection
  const std::streamsize prepended_size = filesize + 512;
  const std::streamsize buffer_size =
      ((prepended_size % ep0_transaction_size != 0) &&
       (prepended_size % 64 == 0))
          ? prepended_size + 1
          : prepended_size;
  std::vector<char> bitstream(buffer_size, 0);
  if (!file.read(bitstream.data() + 512, filesize))
    throw ztex_error(std::string("could not load bitstream ") +
                     bitstream_location.string());

  // check bitorder since Vivado has two possible formats (serial and parallel
  // download) - we need flipped bitorder for parallel download
  // xilinx bitstream files carry a synchronization word
  //
  // see Series 7 Configuration Guide, p. 83
  // https://www.xilinx.com/support/documentation/user_guides/ug470_7Series_Config.pdf
  std::vector<char> flipped_sync{'\x55', '\x99', '\xaa', '\x66'};
  std::vector<char> normal_sync{'\xaa', '\x99', '\x55', '\x66'};
  auto index_flipped = std::search(begin(bitstream), end(bitstream),
                                   begin(flipped_sync), end(flipped_sync));
  auto index_normal = std::search(begin(bitstream), end(bitstream),
                                  begin(normal_sync), end(normal_sync));
  if (index_flipped == end(bitstream) && index_normal == end(bitstream))
    throw ztex_error(
        "can't determine bitorder of bitstream, expected mark missing");

  if (index_normal < index_flipped) {
    for (auto &c : bitstream) {
      c = numeric_utils::flip_bits(c);
    }
  }

  // TODO implement fast config
  using namespace std::literals::chrono_literals;
  intf.control_write(usb::type::vendor, usb::recipient::device,
                     static_cast<uint8_t>(commands::fpga_reset), 0, 0,
                     gsl::span<uint8_t>{}, 1500ms);

  utils::in_chunks<char>(bitstream, ep0_transaction_size, [&](auto chunk) {
    const auto size{chunk.size()};
    auto sent{intf.control_write(
        usb::type::vendor, usb::recipient::device,
        static_cast<uint8_t>(commands::fpga_send), 0, 0,
        gsl::span((uint8_t *)chunk.data(), chunk.size()), 1000ms)};
    if (sent != chunk.size())
      throw ztex_error("Error transferring bitstream");
  });
  // TODO do we need to send additional frame if no additional byte was added?
  // would also be missing in the original source

  if (!is_fpga_configured(intf))
    throw ztex_error("FPGA not configured after bitstream download");
}

uint8_t ctrl_gpio(usb::interface &intf, uint8_t mask, uint8_t value) {
  using namespace std::literals::chrono_literals;
  std::array<uint8_t, 8> buffer;
  intf.control_read(usb::type::vendor, usb::recipient::device,
                    static_cast<uint8_t>(commands::gpio_ctrl), value, mask,
                    buffer, 1500ms);
  return buffer[0];
}

void reset_fpga(usb::interface &intf, bool leave) {
  using namespace std::literals::chrono_literals;
  intf.control_write(usb::type::vendor, usb::recipient::device,
                     static_cast<uint8_t>(commands::reset_toggle),
                     leave ? 1 : 0, 0, {}, 1500ms);
}

// TODO LSI functions

} // namespace ztex