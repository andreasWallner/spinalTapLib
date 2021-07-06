#ifndef spinaltap_spinaltap_h
#define spinaltap_spinaltap_h

#include "gsl/gsl"
#include <cstdint>
#include <filesystem>

namespace usb {
class out_endpoint;
class in_endpoint;
class interface;
} // namespace usb

namespace spinaltap {

enum class cmd : uint8_t {
  write = 0x01,
  read = 0x02,
  flush = 0xff,
  writeStream8 = 0x03,
  readStream8 = 0x04
};
class device {
public:
  device(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep);

  uint32_t readRegister(uint32_t address);
  void readStream(uint32_t address, gsl::span<std::byte> data);

  void writeRegister(uint32_t address, uint32_t value);
  void writeStream(uint32_t address, gsl::span<std::byte> data);
  void
  writeRegisters(const std::vector<std::pair<uint32_t, uint32_t>> &toWrite);

  void readModifyWrite(uint32_t address, uint32_t mask, uint32_t value);

private:
  usb::out_endpoint &out_ep_;
  usb::in_endpoint &in_ep_;
};

namespace endian {
void store(uint16_t v, gsl::span<uint8_t, 2> buffer) noexcept;
void store(uint32_t v, gsl::span<uint8_t, 4> buffer) noexcept;

template <typename T> T load(gsl::span<uint8_t> buffer) noexcept;
template <> uint16_t inline load(gsl::span<uint8_t> buffer) noexcept {
  return (buffer[1] << 8) | buffer[0];
}
template <> uint32_t inline load(gsl::span<uint8_t> buffer) noexcept {
  return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}
} // namespace endian

namespace control {
void load_bitstream(const usb::interface &intf,
                    const std::filesystem::path &location);
}
} // namespace spinaltap

#endif