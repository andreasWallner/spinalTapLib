#include "spinaltap.hpp"

#include <chrono>
#include <cstdint>

namespace spinaltap {
namespace control {
enum class commands : uint8_t {
  reset = 0xf0,
  get_configuration_state = 0xfc,
  start_configuration = 0xfd,
  write_configuration_chunk = 0xfe,
  finish_configuration = 0xff
};

void load_bitstream(usb::interface &intf,
                    const std::filesystem::path &location) {
  using namespace std::chrono_literals;
  intf.control_write(usb::type::vendor, usb::recipient::device,
                     static_cast<uint8_t>(commands::start_configuration), 0, 0,
                     {}, 1500ms);
  uint8_t bytes[] = {0x11, 0x22, 0x33, 0x44};
  intf.control_write(usb::type::vendor, usb::recipient::device,
                     static_cast<uint8_t>(commands::write_configuration_chunk),
                     0, 0, gsl::span<uint8_t>(bytes), 1500ms);
  intf.control_write(usb::type::vendor, usb::recipient::device,
                     static_cast<uint8_t>(commands::finish_configuration), 0, 0,
                     {}, 1500ms);
  std::array<uint8_t, 1> response;
  intf.control_read(usb::type::vendor, usb::recipient::device,
                    static_cast<uint8_t>(commands::get_configuration_state), 0,
                    0, response, 1500ms);
  fmt::print("read: {}", response[0]);
}
} // namespace control
} // namespace spinaltap