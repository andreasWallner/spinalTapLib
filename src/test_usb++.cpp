#include "clipp.h"
#include "fmt/core.h"
#include "libusb++/libusb++.hpp"
#include "libusb++/utils.hpp"
#include "numeric_utils.hpp"
#include "random.hpp"
#include "ztexpp.hpp"

#include "utils.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>

using namespace numeric_utils;

static void loopbackTest(usb::interface &intf, ztex::dev_info &info);
static void readValueTest(usb::interface &intf, ztex::dev_info &info);
static bool interactiveShell(usb::interface &intf, usb::in_endpoint &in_ep,
                             usb::out_endpoint &out_ep);

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
            loop = interactiveShell(intf, in_ep, out_ep);
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

namespace util {
namespace endian {
void store(uint16_t v, gsl::span<uint8_t, 2> buffer) {
  buffer[0] = static_cast<uint8_t>(v);
  buffer[1] = static_cast<uint8_t>(v >> 8);
}
void store(uint32_t v, gsl::span<uint8_t, 4> buffer) {
  buffer[0] = static_cast<uint8_t>(v);
  buffer[1] = static_cast<uint8_t>(v >> 8);
  buffer[2] = static_cast<uint8_t>(v >> 16);
  buffer[3] = static_cast<uint8_t>(v >> 24);
}
template <typename T> T load(gsl::span<uint8_t> buffer);
template <> uint16_t load(gsl::span<uint8_t> buffer) {
  return (buffer[1] << 8) | buffer[0];
}
template <> uint32_t load(gsl::span<uint8_t> buffer) {
  return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}
} // namespace endian
} // namespace util

namespace bus_master {
enum class cmd : uint8_t { write = 0x01, read = 0x02, flush = 0xff };
}
namespace peripherals {
namespace gpio {
constexpr uint16_t in = 0x0000;
constexpr uint16_t out = 0x0004;
constexpr uint16_t enable = 0x0008;
} // namespace gpio
namespace pwm0 {
constexpr uint16_t conf = 0x0100;
constexpr uint16_t prescaler = 0x0104;
constexpr uint16_t max_cnt = 0x0108;
constexpr std::array<uint16_t, 10> level = {0x010c, 0x0110, 0x0114, 0x0118,
                                            0x011c, 0x0120, 0x0124, 0x0128,
                                            0x012c, 0x0130};
} // namespace pwm0
} // namespace peripherals

static void writeRegister(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep,
                          uint16_t address, uint32_t value) {
  std::array<uint8_t, 8> msg;
  msg[0] = 0;
  msg[1] = static_cast<uint8_t>(bus_master::cmd::write);
  util::endian::store(address, gsl::span(msg).subspan<2, 2>());
  util::endian::store(value, gsl::span(msg).subspan<4, 4>());
  fmt::print("sending: ");
  utils::hexdump(msg);
  std::flush(std::cout);

  out_ep.bulk_write_all(msg);
  std::array<uint8_t, 2> reply;
  in_ep.bulk_read_all(reply, std::chrono::milliseconds(500));
  utils::hexdump(reply);
}

static uint32_t readRegister(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep,
                             uint16_t address) {
  std::array<uint8_t, 4> msg;
  msg[0] = 0;
  msg[1] = static_cast<uint8_t>(bus_master::cmd::read);
  util::endian::store(address, gsl::span(msg).subspan<2, 2>());
  /*fmt::print("sending: ");
  utils::hexdump(msg);
  std::flush(std::cout);*/
  out_ep.bulk_write_all(msg);

  std::array<uint8_t, 6> reply;
  in_ep.bulk_read_all(reply, std::chrono::milliseconds(500));
  /*fmt::print("received: ");
  utils::hexdump(reply);*/
  return util::endian::load<uint32_t>(gsl::span(reply).subspan<2, 4>());
}

static void knightrider_update(std::array<uint32_t, 10> &levels, int pos) {
  constexpr auto factor = 0.6f;
  for (auto &level : levels)
    level = static_cast<uint32_t>(level * factor);
  levels[pos] = 100;
}

#include <thread>
static void knightrider(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep,
                        int milliseconds, int duration) {
  writeRegister(out_ep, in_ep, peripherals::pwm0::conf, 0);
  writeRegister(out_ep, in_ep, peripherals::pwm0::prescaler, 100);
  writeRegister(out_ep, in_ep, peripherals::pwm0::max_cnt, 100);
  for (auto level : peripherals::pwm0::level)
    writeRegister(out_ep, in_ep, level, 0);
  writeRegister(out_ep, in_ep, peripherals::pwm0::conf, 1);

  std::array<uint32_t, 10> levels{0};
  uint16_t pos = 0;
  bool right = true;
  do {
    pos = right ? pos + 1 : pos - 1;

    if (pos == 9) {
      right = false;
    } else if (pos == 0) {
      right = true;
    }

    knightrider_update(levels, pos);

    for (int i = 0; i < 10; i++)
      writeRegister(out_ep, in_ep, peripherals::pwm0::level[i], levels[i]);
    fmt::print("remaining: {}\n", duration);
    std::flush(std::cout);

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  } while (duration--);
}

std::vector<uint8_t> randomBrightColor() {
  return std::vector<uint8_t>{
      (std::random_device()() & 1) != 0 ? (uint8_t)255 : (uint8_t)0,
      (std::random_device()() & 1) != 0 ? (uint8_t)255 : (uint8_t)0,
      (std::random_device()() & 1) != 0 ? (uint8_t)255 : (uint8_t)0};
}

static void writeRegisters(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep,
                           gsl::span<uint8_t, 3> values) {
  static uint8_t source = 0;
  std::array<uint8_t, 8 * 3> msg;
  for (int i = 0; i < 3; i++) {
    msg[i * 8] = source++;
    msg[i * 8 + 1] = static_cast<uint8_t>(bus_master::cmd::write);
    util::endian::store(
        0x04 * (i + 1),
        gsl::span<uint8_t, 8>(msg.data() + i * 8, 8).subspan<2, 2>());
    util::endian::store(
        values[i],
        gsl::span<uint8_t, 8>(msg.data() + i * 8, 8).subspan<4, 4>());
  }
  /*fmt::print("sending: ");
  utils::hexdump(msg);
  std::flush(std::cout);*/

  out_ep.bulk_write_all(msg);
  std::array<uint8_t, 2 * 3> reply;
  in_ep.bulk_read_all(reply, std::chrono::milliseconds(500));
  //utils::hexdump(reply);
}

template <class Rep, class Period>
static void colorfade(usb::out_endpoint &out_ep, usb::in_endpoint &in_ep,
                      std::chrono::duration<Rep, Period> rate, unsigned int duration,
                      unsigned int fadespeed = 10) {
  auto current_color = std::vector<uint8_t>{255, 255, 255};
  writeRegisters(out_ep, in_ep, gsl::span<uint8_t, 3>(current_color));
  auto next_color = randomBrightColor();
  do {
    if (std::equal(begin(current_color), end(current_color), begin(next_color), end(next_color))) {
      next_color = randomBrightColor();
    }
    for (size_t i = 0; i < 3; i++) {
      if (current_color[i] == next_color[i])
        continue;
      int delta = current_color[i] > next_color[i]
                      ? -static_cast<int>(fadespeed)
                      : static_cast<int>(fadespeed);
      current_color[i] =
          static_cast<uint8_t>(std::clamp(current_color[i] + delta, 0, 255));
    }
    writeRegisters(out_ep, in_ep, gsl::span<uint8_t, 3>(current_color));
    std::this_thread::sleep_for(rate);
  } while (duration--);
}

static bool interactiveShell(usb::interface &intf, usb::in_endpoint &in_ep,
                             usb::out_endpoint &out_ep) {
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
    writeRegister(out_ep, in_ep, address, value);
    return true;
  } else if (pieces.at(0) == "read" || pieces.at(0) == "r") {
    if (pieces.size() != 2)
      return true;
    int address = to_int(pieces.at(1));
    int value = readRegister(out_ep, in_ep, address);
    fmt::print("0x{0:04x} = 0x{1:08x} ({1})\n", address, value);
    return true;
  } else if (pieces.at(0) == "colorfade") {
    if (pieces.size() != 3)
      return true;
    int rate = to_int(pieces.at(1));
    int duration = to_int(pieces.at(2));
    colorfade(out_ep, in_ep, std::chrono::milliseconds(rate), duration);
    return true;
  } else {
    fmt::print("invalid command\n");
    return true;
  }
}
