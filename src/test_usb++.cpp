#include "clipp.h"
#include "libusb++/libusb++.hpp"
#include "libusb++/logging.hpp"
#include "libusb++/utils.hpp"
#include "numeric_utils.hpp"
#include "random.hpp"
#include "spinaltap.hpp"
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
  using filter_predicate =
      std::function<bool(const usb::device &, libusb_device_descriptor &)>;
  std::vector<filter_predicate> filters;
  bool tryStuff = false;
  bool verbose = false;
  bool download = false;
  bool test_txrx = false;
  bool test_rx = false;
  bool interactive = false;
  std::string bitstream;

  auto cli =
      (clipp::option("-t").doc("Try stuff").set(tryStuff),
       clipp::option("-b").doc("Filter by bus ID") &
           clipp::number("bus")([&](std::string_view s) {
             filters.push_back(
                 [=](const usb::device &d, libusb_device_descriptor &) {
                   return d.bus_number() == as_integer<int, 10>(s);
                 });
           }),
       clipp::option("-d").doc("Filter by device ID") &
           clipp::number("device")([&](std::string_view s) {
             filters.push_back(
                 [=](const usb::device &d, libusb_device_descriptor &) {
                   return d.device_address() == as_integer<int, 10>(s);
                 });
           }),
       clipp::option("-v").doc("Filter by vendor ID") &
           clipp::value(is_number<uint16_t, 16>,
                        "vendor")([&](std::string_view s) {
             filters.push_back(
                 [=](const usb::device &, libusb_device_descriptor &dd) {
                   return dd.idVendor == as_integer<uint16_t, 16>(s);
                 });
           }),
       clipp::option("-p").doc("Filter by product ID") &
           clipp::value(is_number<uint16_t, 16>,
                        "product")([&](std::string_view s) {
             filters.push_back(
                 [=](const usb::device &, libusb_device_descriptor &dd) {
                   return dd.idProduct == as_integer<uint16_t, 16>(s);
                 });
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

  try {

    auto result = clipp::parse(argc, argv, cli);

    if (result.any_error()) {
      std::cout << "Usage:\n"
                << clipp::usage_lines(cli, "progname") << "\nOptions:\n"
                << clipp::documentation(cli) << '\n';
      exit(-1);
    }

    usb::context usb_ctx;
    usb_ctx.set_log_level(usb::log_level::none);
    {
      const usb::device_list list{usb_ctx};
      std::vector<usb::device> matches;
      std::copy_if(
          list.begin(), list.end(), std::back_inserter(matches), [&](auto dev) {
            auto dd = dev.device_descriptor();
            return std::all_of(begin(filters), end(filters),
                               [&](filter_predicate f) { return f(dev, dd); });
          });
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

std::vector<uint32_t> randomBrightColor() {
  return std::vector<uint32_t>{
      (std::random_device()() & 1) != 0 ? 255U : 0U,
      (std::random_device()() & 1) != 0 ? 255U : 0U,
      (std::random_device()() & 1) != 0 ? 255U : 0U};
}

template <class Rep, class Period>
static void colorfade(spinaltap::device &device,
                      std::chrono::duration<Rep, Period> rate,
                      unsigned int duration, unsigned int fadespeed = 10) {
  auto current_color = std::vector<uint8_t>{255, 255, 255};
  auto current_regs = std::vector<std::pair<uint32_t, uint32_t>>{
      {0x00, 0xff}, {0x04, 0xff}, {0x08, 0xff}};
  device.writeRegisters(current_regs);
  auto next_color = randomBrightColor();
  do {
    if (current_regs[0].second == next_color[0] &&
        current_regs[1].second == next_color[1] &&
        current_regs[2].second == next_color[2])
      next_color = randomBrightColor();

    for (size_t i = 0; i < 3; i++) {
      if (current_regs[i].second == next_color[i])
        continue;
      int delta = current_regs[i].second > next_color[i]
                      ? -static_cast<int>(fadespeed)
                      : static_cast<int>(fadespeed);
      current_regs[i].second = static_cast<uint8_t>(
          std::clamp(current_regs[i].second + delta, 0U, 255U));
    }
    device.writeRegisters(current_regs);
    std::this_thread::sleep_for(rate);
  } while (duration--);
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
    return true;
  } else if (pieces.at(0) == "read" || pieces.at(0) == "r") {
    if (pieces.size() != 2)
      return true;
    int address = to_int(pieces.at(1));
    int value = device.readRegister(address);
    fmt::print("0x{0:04x} = 0x{1:08x} ({1})\n", address, value);
    return true;
  } else if (pieces.at(0) == "colorfade") {
    if (pieces.size() != 3)
      return true;
    int rate = to_int(pieces.at(1));
    int duration = to_int(pieces.at(2));
    colorfade(device, std::chrono::milliseconds(rate), duration);
    return true;
  } else {
    fmt::print("invalid command\n");
    return true;
  }
}
