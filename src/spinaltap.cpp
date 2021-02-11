#include "spinaltap.hpp"
#include "utils.hpp"

#include <chrono>
#include <cstdint>

namespace spinaltap {
void make_write(gsl::span<uint8_t, 8> dest, uint32_t address, uint32_t value) {
  if (address >= std::numeric_limits<uint8_t>::max())
    throw std::logic_error("impossible register address");

  dest[0] = 0;
  dest[1] = static_cast<uint8_t>(cmd::write);
  endian::store(static_cast<uint16_t>(address), dest.subspan<2, 2>());
  endian::store(value, dest.subspan<4, 4>());
}

device::device(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep)
    : out_ep_(out_ep), in_ep_(in_ep) {}

uint32_t device::readRegister(uint32_t address) {
  if (address >= std::numeric_limits<uint8_t>::max())
    throw std::logic_error("impossible register address");

  std::array<uint8_t, 4> msg;
  msg[0] = 0;
  msg[1] = static_cast<uint8_t>(cmd::read);
  endian::store(address, gsl::span(msg).subspan<2, 2>());
  out_ep_.bulk_write_all(msg);

  std::array<uint8_t, 6> reply;
  in_ep_.bulk_read_all(reply, std::chrono::milliseconds(500));
  return endian::load<uint32_t>(gsl::span(reply).subspan<2, 4>());
}

void device::writeRegister(uint32_t address, uint32_t value) {
  std::array<uint8_t, 8> msg;
  make_write(msg, address, value);

  out_ep_.bulk_write_all(msg);
  std::array<uint8_t, 2> reply;
  in_ep_.bulk_read_all(reply, std::chrono::milliseconds(500));
}

void device::writeRegisters(
    const std::vector<std::pair<uint32_t, uint32_t>> &toWrite) {
  // TODO: replace with uninitialized memory
  std::vector<uint8_t> msg(toWrite.size() * 8, static_cast<uint8_t>(0));
  for (int i = 0; i < toWrite.size(); i++) {
    make_write(gsl::span<uint8_t, 8>(msg.data() + i * 8, 8), toWrite[i].first,
               toWrite[i].second);
  }
  out_ep_.bulk_write_all(msg);

  msg.resize(toWrite.size() * 2);
  in_ep_.bulk_read_all(msg, std::chrono::milliseconds(500));
}

namespace endian {
void store(uint16_t v, gsl::span<uint8_t, 2> buffer) noexcept {
  buffer[0] = static_cast<uint8_t>(v);
  buffer[1] = static_cast<uint8_t>(v >> 8);
}
void store(uint32_t v, gsl::span<uint8_t, 4> buffer) noexcept {
  buffer[0] = static_cast<uint8_t>(v);
  buffer[1] = static_cast<uint8_t>(v >> 8);
  buffer[2] = static_cast<uint8_t>(v >> 16);
  buffer[3] = static_cast<uint8_t>(v >> 24);
}
} // namespace endian

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