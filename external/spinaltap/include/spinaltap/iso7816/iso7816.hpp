#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include "GSL/gsl"

#include "spinaltap.hpp"

namespace spinaltap::iso7816 {

enum class start_receive_t { yes, no };
enum class reset_t { cold, warm };
enum class rx_flush_t { flush, keep };
enum class tx_flush_t { flush, keep };
enum class module_state_t { idle, state_change, tx, rx };
enum class interface_state_t { active, reset, inactive, clockstop };

class master {
public:
  using duration = std::chrono::nanoseconds;
  using frequency = uint64_t;

private:
  device &device_;
  uint32_t baseAddress_;
  frequency clock_freq_;
  size_t rx_buffer_size_;
  size_t tx_buffer_size_;

public:
  master(device &device, uint32_t baseAddress)
      : device_(device), baseAddress_(baseAddress) {
    read_cache();
  }

  frequency module_clock_frequency() const;
  size_t rx_buffer_size() const;
  size_t tx_buffer_size() const;

  void activate(start_receive_t receive,
                rx_flush_t rx_flush = rx_flush_t::flush);
  void deactivate();
  void reset();
  void stop_clock();
  void wait_until_idle();

  std::tuple<module_state_t, interface_state_t> state() const;

  // TODO timing
  std::array<duration, 6> reset_timing();
  std::array<duration, 6> set_reset_timing(std::array<duration, 6>);
  void reset_and_activate(reset_t reset, start_receive_t receive);

  std::vector<std::byte> receive();
  bool receive(gsl::span<std::byte> buffer,
               duration timeout = std::chrono::seconds(1));
  std::vector<std::byte> receive(std::size_t n,
                                 duration timeout = std::chrono::seconds(1));
  template <std::size_t N>
  std::array<std::byte, N> receive(duration timeout = std::chrono::seconds(1));
  std::vector<std::byte> receiveAll();
  size_t receive(gsl::span<std::byte> buffer);
  void transmit(gsl::span<std::byte> bytes,
                start_receive_t receive = start_receive_t::yes,
                rx_flush_t rx_flush = rx_flush_t::flush);

  bool character_repetition();
  void set_character_repetition(bool charrep);

  uint32_t character_guard_time();
  void set_character_guard_time(uint32_t cgt);

  duration iso_clock() const;
  duration set_iso_clock(frequency f);

  duration block_timeout() const;
  duration set_block_timeout(duration);
  duration character_timeout() const;
  duration set_character_timeout(duration);

  uint16_t rx_fifo_available() const;
  uint16_t tx_fifo_free() const;

private:
  void read_cache();
};

} // namespace spinaltap::iso7816
