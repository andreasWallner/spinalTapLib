#include "clipp.h"
#include "libusb++/device_filter.hpp"
#include "libusb++/libusb++.hpp"
#include "libusb++/logging.hpp"
#include "libusb++/utils.hpp"
#include "numeric_utils.hpp"
#include "random.hpp"
#include "spinaltap.hpp"
#include "spinaltap/gpio/gpio.hpp"
#include "spinaltap/iomux/iomux.hpp"
#include "spinaltap/iso7816/iso7816.hpp"
#include "spinaltap/logging.hpp"
#include "spinaltap/pwm/pwm.hpp"
#include "spinaltap/pwm/registers.hpp"
#include "spinaltap/spi/spi.hpp"
#include "ztexpp.hpp"

#include "fmt/chrono.h"
#include "fmt/core.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>

using namespace numeric_utils;

static void loopbackTest(usb::interface &intf, ztex::dev_info &info);
static void readValueTest(usb::interface &intf, ztex::dev_info &info);
static bool interactiveShell(spinaltap::device &device);

int main(int argc, char *argv[]) {
  usb::device_filter filter;
  bool tryStuff = false;
  bool verbose = true;
  bool download = false;
  bool test_txrx = false;
  bool test_rx = false;
  bool interactive = true;
  std::string bitstream;

  // for easier debugging
  filter.product_id(1337);
  auto cli =
      (clipp::option("-t").doc("Try stuff").set(tryStuff),
       clipp::option("-b").doc("Filter by bus ID") &
           clipp::number("bus")(
               [&](std::string_view s) { filter.bus(as_integer<int, 10>(s)); }),
       clipp::option("-d").doc("Filter by device ID") &
           clipp::number("device")([&](std::string_view s) {
             filter.device_address(as_integer<int, 10>(s));
           }),
       clipp::option("-v").doc("Filter by vendor ID") &
           clipp::value(is_number<uint16_t, 16>,
                        "vendor")([&](std::string_view s) {
             filter.vendor_id(as_integer<uint16_t, 16>(s));
           }),
       clipp::option("-p").doc("Filter by product ID") &
           clipp::value(is_number<uint16_t, 16>,
                        "product")([&](std::string_view s) {
             filter.product_id(as_integer<uint16_t, 16>(s));
           }),
       clipp::option("--verbose").doc("Print detail information").set(verbose),
       clipp::option("--download").doc("Try download").set(download) &
           clipp::value("bitstream", bitstream),
       clipp::option("--test-txrx").doc("Run TX/RX test").set(test_txrx),
       clipp::option("--test-rx").doc("Run RX speedtest").set(test_rx),
       clipp::option("--interactive")
           .doc("Run interactive shell")
           .set(interactive));

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  usb::logging::logger->sinks().push_back(console_sink);
  usb::logging::logger->set_level(spdlog::level::debug);
  spinaltap::logging::logger->sinks().push_back(console_sink);
  spinaltap::logging::logger->set_level(spdlog::level::debug);

  try {

    auto result = clipp::parse(argc, argv, cli);

    if (result.any_error()) {
      std::cout << "Usage:\n"
                << clipp::usage_lines(cli, "progname") << "\nOptions:\n"
                << clipp::documentation(cli) << '\n';
      exit(-1);
    }

    usb::context usb_ctx;
    usb_ctx.set_log_level(usb::log_level::info);
    {
      const usb::device_list list{usb_ctx};
      std::vector<usb::device> matches{filter.filter(list)};
      for (const auto &dev : matches) {
        if (!verbose)
          usb::utils::print_short(dev);
        else
          usb::utils::print_recursive(dev);
      }

      if (tryStuff) {
        if (matches.size() != 1) {
          fmt::print("invalid number of devices ({}) - exit", matches.size());
          return -1;
        }

        try {
          usb::device_handle dh{matches[0]};
          dh.reset();
          dh.set_configuration(1);

        } catch (usb::usb_error e) {
          fmt::print("  Error ({})\n", e.what());
        }
      }

      if (download || test_txrx || test_rx) {
        if (matches.size() != 1) {
          fmt::print("invalid number of devices ({}) - exit", matches.size());
          return -1;
        }

        usb::device_handle dh{matches[0]};
        dh.reset();
        dh.set_configuration(1);

        try {
          if (download) {
            usb::interface intf{dh, 0};
            auto info = ztex::device_info(intf);
            // ztex::upload_bitstream(intf, info, "C:/work/tmp/Board_2_14.bit");
            ztex::upload_bitstream(intf, info, bitstream);
          }

          if (test_txrx) {
            usb::interface intf{dh, 0};
            auto info = ztex::device_info(intf);
            loopbackTest(intf, info);
          }
          if (test_rx) {
            usb::interface intf{dh, 0};
            auto info = ztex::device_info(intf);
            readValueTest(intf, info);
          }

        } catch (const std::exception &e) {
          fmt::print(e.what());
        }
      }
      if (interactive) {
        if (matches.size() != 1) {
          fmt::print("invalid number of devices ({}) - exit", matches.size());
          return -1;
        }
        usb::device_handle dh{matches[0]};
        dh.reset();
        dh.set_configuration(1);

        usb::interface intf{dh, 0};
        // auto info = ztex::device_info(intf);
        bool loop = true;
        do {
          try {
            usb::in_endpoint in_ep{intf, 1};
            usb::out_endpoint out_ep{intf, 2};
            spinaltap::device device{out_ep, in_ep};
            loop = interactiveShell(device);
          } catch (std::runtime_error &e) {
            fmt::print("Error: {}\n", e.what());
          } catch (usb::usb_error &e) {
            fmt::print("USB Error: {}\n", e.what());
          }
        } while (loop);
      }
    }
  } catch (const std::exception &e) {
    fmt::print("error: {}", e.what());
  }
}

static void loopbackTest(usb::interface &intf, ztex::dev_info &info) {
  ztex::reset_fpga(intf, true);
  (void)ztex::ctrl_gpio(intf, 0b0111, 0b0000);
  ztex::reset_fpga(intf, false);

  std::vector<uint8_t> txbuf(1024, 0);
  xorshift16 xs16{};
  std::generate(begin(txbuf), end(txbuf),
                [&] { return static_cast<uint8_t>(xs16()); });

  usb::out_endpoint out_ep{intf, info.default_out_ep};
  usb::in_endpoint in_ep{intf, info.default_in_ep};

  out_ep.bulk_write_all(txbuf, std::chrono::milliseconds{4000});
  out_ep.bulk_write_all(txbuf, std::chrono::milliseconds{4000});
  fmt::print("sent {}\n", txbuf.size());

  std::vector<uint8_t> rxbuf(txbuf.size());
  int received = in_ep.bulk_read(rxbuf, std::chrono::milliseconds{4000});
  fmt::print("received {}\n", received);
  auto first_error =
      std::mismatch(begin(txbuf), end(txbuf), begin(rxbuf), end(rxbuf));
  if (first_error.first != end(txbuf) || first_error.second != end(rxbuf)) {
    fmt::print("rx error at byte {}\n",
               std::distance(begin(txbuf), first_error.first));
    fmt::print("TX ({}):\n", txbuf.size());
    utils::hexdump(txbuf, 32);
    fmt::print("\nRX ({}):\n", rxbuf.size());
    utils::hexdump(rxbuf, 32);
  } else {
    fmt::print("DONE: OK\n");
  }
}

static void readValueTest(usb::interface &intf, ztex::dev_info &info) {
  using namespace std::literals::chrono_literals;
  std::vector<uint8_t> mbuf(4 * 1024 * 1024, 0);

  ztex::reset_fpga(intf, false);
  ztex::ctrl_gpio(intf, 0b0111, 0b0001);

  usb::in_endpoint in_ep{intf, info.default_in_ep};
  int size{0};
  int errors{0};
  xorshift16 xs{0xc181}; // seed so that first result is 1, matching HW
  while (size < 2000000) {
    int transferred = in_ep.bulk_read(mbuf, 2000ms);

    fmt::print("transferred: {}\n", transferred);
    if (transferred % 2)
      throw std::runtime_error("unexpected transfer size != 2 * n");

    uint16_t *mbuf16 = reinterpret_cast<uint16_t *>(mbuf.data());
    while (transferred >= 2) {
      size++;
      uint16_t expected = xs();
      if (*mbuf16 != expected) {
        fmt::print("{}: 0x{:04x} != 0x{:04x}\n", size, *mbuf16, expected);
        if (errors++ > 20)
          return;
      } else if (size < 20) {
        fmt::print("{}: 0x{:04x} == 0x{:04x}\n", size, *mbuf16, expected);
      }
      mbuf16++;
      transferred -= 2;
    }
  }

  if (errors == 0)
    fmt::print("DONE: OK\n");
}

int to_int(std::string num) {
  char *end;
  int val = strtoll(num.c_str(), &end, 0);
  if (end != num.c_str() + num.size() || num.size() == 0)
    throw std::runtime_error(std::string("invalid number: ") + num);
  return val;
}

std::array<uint8_t, 3> randomBrightColor() {
  return {(std::random_device()() & 1) != 0 ? uint8_t(255) : uint8_t(0),
          (std::random_device()() & 1) != 0 ? uint8_t(255) : uint8_t(0),
          (std::random_device()() & 1) != 0 ? uint8_t(255) : uint8_t(0)};
}

template <class Rep, class Period>
static void colorfade(spinaltap::pwm::pwm &pwm,
                      std::chrono::duration<Rep, Period> rate,
                      unsigned int duration, unsigned int fadespeed = 10) {
  auto current_color = std::vector<uint8_t>{255, 255, 255};
  auto current_regs = std::array<uint8_t, 3>{0xff, 0xff, 0xff};

  pwm.set_widths(current_regs);

  auto next_color = randomBrightColor();
  do {
    if (current_regs[0] == next_color[0] && current_regs[1] == next_color[1] &&
        current_regs[2] == next_color[2])
      next_color = randomBrightColor();

    for (size_t i = 0; i < 3; i++) {
      if (current_regs[i] == next_color[i])
        continue;
      int delta = current_regs[i] > next_color[i] ? -static_cast<int>(fadespeed)
                                                  : static_cast<int>(fadespeed);
      current_regs[i] = static_cast<uint8_t>(
          std::clamp(static_cast<int>(current_regs[i]) + delta, 0, 255));
    }
    pwm.set_widths(current_regs);
    std::this_thread::sleep_for(rate);
  } while (duration--);
}

namespace module_base {
constexpr uint32_t pwm = 0x0000;
constexpr uint32_t gpio0 = 0x0100;
constexpr uint32_t gpio1 = 0x0200;
constexpr uint32_t mux = 0x0300;
constexpr uint32_t iso7816 = 0x0400;
constexpr uint32_t spi = 0x500;
} // namespace module_base

namespace mux_input {
constexpr uint32_t none = 0;
constexpr uint32_t gpio0 = 1;
constexpr uint32_t gpio1 = 2;
constexpr uint32_t iso7816 = 3;
constexpr uint32_t spi = 4;
}; // namespace mux_input

static void iotest(spinaltap::device &device) {
  spinaltap::iomux::iomux mux(device, module_base::mux);
  spinaltap::gpio::gpio gpio0(device, module_base::gpio0);
  spinaltap::gpio::gpio gpio1(device, module_base::gpio1);

  mux.connect(mux_input::gpio0, 0);
  mux.connect(mux_input::gpio1, 1);

  gpio0.set_write(0x00);
  gpio0.set_write_enable(0xff);
  gpio1.set_write(0x00);
  gpio1.set_write_enable(0xff);

  gpio0.set_write(0x01);
  gpio0.set_write(0x00);
  gpio0.set_write(0xff);
  fmt::print("{}", gpio0.write());
  gpio0.set_write(0x00);

  gpio1.set_write(0x01);
  gpio1.set_write(0x00);
  gpio1.set_write(0xff);
  gpio1.set_write(0x00);
}

static void spitest(spinaltap::device &device) {
  spinaltap::iomux::iomux mux(device, module_base::mux);
  spinaltap::spi::master spi(device, module_base::spi);

  mux.connect(mux_input::spi, 0);
  spi.configure(spinaltap::spi::cpol::idle_high,
                spinaltap::spi::cpha::first_edge_latches, 1.0e3);

  std::vector<uint8_t> buffer = {0x55, 0xff, 0x10, 0x01, 0x80};
  spi.transceive(buffer, buffer);
}

constexpr int popcount(std::byte b) noexcept {
  constexpr std::byte operator""_b(unsigned long long x) {
    return static_cast<std::byte>(x);
  };
  return std::to_integer<int>((b >> 0) & 1_b) +
         std::to_integer<int>((b >> 1) & 1_b) +
         std::to_integer<int>((b >> 2) & 1_b) +
         std::to_integer<int>((b >> 3) & 1_b) +
         std::to_integer<int>((b >> 4) & 1_b) +
         std::to_integer<int>((b >> 5) & 1_b) +
         std::to_integer<int>((b >> 6) & 1_b) +
         std::to_integer<int>((b >> 7) & 1_b);
}

std::vector<std::byte>
receive_atr(std::function<bool(gsl::span<std::byte> buffer)> recv_func) {
  constexpr std::byte operator""_b(unsigned long long x) {
    return static_cast<std::byte>(x);
  };
  constexpr std::size_t max_atr_size = 32;
  std::array<std::byte, max_atr_size> memory;
  gsl::span<std::byte> remaining(memory);
  bool needs_tck = false;

  if (!recv_func(remaining.first(2)))
    return {};

  if (remaining[0] != 0x3B_b)
    return {};
  auto TDx = remaining[1];
  const auto K = std::to_integer<int>(remaining[1] & 0x0f_b);
  remaining = remaining.subspan(2);

  int next_block_len;
  while ((next_block_len = popcount(TDx & 0xf0_b))) {
    if (next_block_len > remaining.size())
      return {};

    if (!recv_func(remaining.first(next_block_len)))
      return {};

    if ((TDx & 0x80_b) != 0_b)
      TDx = remaining[next_block_len - 1];
    else
      TDx = 0_b;

    if ((TDx & 0x0f_b) != 0_b)
      needs_tck = true;

    remaining = remaining.subspan(next_block_len);
  }

  if (!recv_func(remaining.first(K)))
    return {};
  remaining = remaining.subspan(K);

  if (needs_tck) {
    if (!recv_func(remaining.first(1)))
      return {};
    remaining = remaining.subspan(1);
  }

  return {memory.begin(), memory.end() - remaining.size()};
}

static void isotest(spinaltap::device &device) {
  using namespace spinaltap::iso7816;
  using namespace std::chrono;

  spinaltap::iomux::iomux mux(device, module_base::mux);
  spinaltap::iso7816::master iso(device, module_base::iso7816);

  mux.connect(mux_input::iso7816, 0);
  iso.set_iso_clock(3700000U);
  iso.set_datarate(9200);
  iso.set_character_timeout(
      round<master::duration>(duration<double>(40000.0 / 3.7e6)));
  iso.disable_block_timeout();

  iso.activate(start_receive_t::yes, rx_flush_t::flush);
  auto atr = receive_atr([&] (gsl::span<std::byte> buffer) {
    return iso.receive(buffer);
  });
}

static void perftest(spinaltap::device &device) {
  constexpr size_t cnt = 100;

  {
    const auto before = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < cnt; i++) {
      device.readRegister(spinaltap::pwm::registers::max);
    }
    const auto after = std::chrono::high_resolution_clock::now();
    const auto duration = after - before;
    using double_duration = std::chrono::duration<double>;
    const double_duration dd = after - before;
    fmt::print("{} reads in {} ms => {}ms / read\n", cnt,
               std::chrono::duration_cast<std::chrono::milliseconds>(dd),
               dd.count() / cnt);
  }
  {
    const auto before = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < cnt; i++) {
      device.writeRegister(spinaltap::pwm::registers::max, 0);
    }
    const auto after = std::chrono::high_resolution_clock::now();
    const auto duration = after - before;
    using double_duration = std::chrono::duration<double>;
    const double_duration dd = after - before;
    fmt::print("{} writes in {} ms => {}ms / write\n", cnt,
               std::chrono::duration_cast<std::chrono::milliseconds>(dd),
               dd.count() / cnt);
  }
}

static bool interactiveShell(spinaltap::device &device) {
  std::string cmdLine;
  fmt::print("> ");
  std::getline(std::cin, cmdLine);
  std::istringstream iss(cmdLine);
  std::vector<std::string> pieces((std::istream_iterator<std::string>(iss)),
                                  std::istream_iterator<std::string>());

  if (pieces.size() == 0)
    return true;

  if (pieces.at(0) == "exit") {
    return false;
  } else if (pieces.at(0) == "write" || pieces.at(0) == "w") {
    if (pieces.size() != 3)
      return true;
    int address = to_int(pieces.at(1));
    int value = to_int(pieces.at(2));
    device.writeRegister(address, value);
  } else if (pieces.at(0) == "read" || pieces.at(0) == "r") {
    if (pieces.size() != 2)
      return true;
    int address = to_int(pieces.at(1));
    int value = device.readRegister(address);
    fmt::print("0x{0:04x} = 0x{1:08x} ({1})\n", address, value);
  } else if (pieces.at(0) == "colorfade") {
    if (pieces.size() != 3)
      return true;
    int rate = to_int(pieces.at(1));
    int duration = to_int(pieces.at(2));
    auto pwm = spinaltap::pwm::pwm{device, 0};
    colorfade(pwm, std::chrono::milliseconds(rate), duration);
  } else if (pieces.at(0) == "iotest") {
    iotest(device);
  } else if (pieces.at(0) == "spitest") {
    spitest(device);
  } else if (pieces.at(0) == "isotest") {
    isotest(device);
  } else if (pieces.at(0) == "perftest") {
    perftest(device);
  } else {
    fmt::print("invalid command\n");
  }

  return true;
}
