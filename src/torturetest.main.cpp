// short program to check buffer handling on the USB device
// side. USB device should NACK once buffers are full an
// no transfer descriptors remain, which is not the case
// for the Xilinx supplied USB driver

#include "fmt/core.h"
#include "gsl/span"
#include "libusb++/libusb++.hpp"
#include "libusb++/utils.hpp"
#include "utils.hpp"
#include <future>

constexpr uint16_t vendor_id = 0x1209;
constexpr uint16_t product_id = 0x1337;

int main(int argc, char *argv[]) {
  using namespace std::chrono_literals;
  try {
    usb::context ctx;
    auto devices = usb::utils::filtered_devices(vendor_id, product_id, ctx);
    if (devices.size() != 1) {
      fmt::print("Invalid number of devices matching VID/PID");
      return -1;
    }
    usb::device_handle dh{devices[0]};
    usb::interface intf{dh, 0};
    usb::out_endpoint out_ep{intf, 2};
    usb::in_endpoint in_ep{intf, 0x81};

    constexpr int cnt = 20;

    auto sender = std::async([&] {
      for (int i = 0; i < cnt; i++) {
        std::array<uint8_t, 5> data = {(uint8_t)(0x11 + i), (uint8_t)(0x22 + i),
                                       (uint8_t)(0x33 + i), (uint8_t)(0x44 + i),
                                       (uint8_t)(0x55 + i)};
        out_ep.bulk_write(data, 20s);
      }
    });

    std::this_thread::sleep_for(5s);
    std::vector<uint8_t> buffer(512);
    for (int i = 0; i < cnt; i++) {
      buffer.resize(in_ep.bulk_read(buffer, 5s));
      utils::hexdump(buffer, 32);
    }
  } catch (const usb::usb_error &e) {
    fmt::print("Error: {}", e.what());
  }
}