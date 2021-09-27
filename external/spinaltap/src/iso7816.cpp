#include "spinaltap/iso7816/iso7816.hpp"
#include "spinaltap/iso7816/registers.hpp"

#include <cmath>

namespace spinaltap::iso7816 {

master::frequency master::module_clock_frequency() const { return clock_freq_; }
std::size_t master::rx_buffer_size() const { return rx_buffer_size_; }
std::size_t master::tx_buffer_size() const { return tx_buffer_size_; }

void master::activate(start_receive_t receive, rx_flush_t rx_flush) {
  if (rx_flush == rx_flush_t::flush) {
    device_.writeRegister(registers::trigger, registers::trigger_rx_flush);
  }

  device_.writeRegister(
      registers::trigger,
      registers::trigger_activate |
          (receive == start_receive_t::yes ? registers::trigger_rx : 0));
}

void master::deactivate() {
  device_.writeRegister(registers::trigger, registers::trigger_deactivate);
}

void master::reset() {
  device_.writeRegister(registers::trigger, registers::trigger_reset);
}

void master::stop_clock() {
  device_.writeRegister(registers::trigger, registers::trigger_stop_clock);
}

constexpr static interface_state_t
to_interface_state(uint32_t status) noexcept {
  switch (static_cast<registers::status_state_t>(
      (status & registers::status_state_mask) >> registers::status_state_pos)) {
  case registers::status_state_t::inactive:
    return interface_state_t::inactive;
  case registers::status_state_t::active:
    return interface_state_t::active;
  case registers::status_state_t::reset:
    return interface_state_t::reset;
  case registers::status_state_t::clockstop:
    return interface_state_t::clockstop;
  }
  return interface_state_t::inactive; // should not happen
}

static module_state_t to_module_state(uint32_t status) {
  if ((status & registers::status_rx_active) != 0)
    return module_state_t::rx;
  else if ((status & registers::status_tx_active) != 0)
    return module_state_t::tx;
  else if ((status & registers::status_change_active) != 0)
    return module_state_t::state_change;
  return module_state_t::idle;
}

std::tuple<module_state_t, interface_state_t> master::state() const {
  auto reg = device_.readRegister(registers::status);
  return {to_module_state(reg), to_interface_state(reg)};
}

master::duration divider_to_duration(uint64_t div, uint64_t freq) {
  auto time{static_cast<double>(div) / freq};
  return std::chrono::duration_cast<master::duration>(
      std::chrono::duration<float>(time));
}

uint32_t duration_to_divider(master::duration duration, uint64_t freq) {
  auto float_seconds{
      std::chrono::duration_cast<std::chrono::duration<float>>(duration)};
  return static_cast<uint32_t>(float_seconds.count() * freq);
}

std::array<master::duration, 6> master::reset_timing() {
  std::array<master::duration, 6> ret;
  ret[0] =
      divider_to_duration(device_.readRegister(registers::ta), clock_freq_);
  ret[1] =
      divider_to_duration(device_.readRegister(registers::tb), clock_freq_);
  ret[2] =
      divider_to_duration(device_.readRegister(registers::te), clock_freq_);
  ret[3] =
      divider_to_duration(device_.readRegister(registers::th), clock_freq_);
  ret[4] = divider_to_duration(device_.readRegister(registers::vcc_offset),
                               clock_freq_);
  ret[5] = divider_to_duration(device_.readRegister(registers::clk_offset),
                               clock_freq_);

  return ret;
}

std::array<master::duration, 6>
master::set_reset_timing(std::array<master::duration, 6> times) {
  device_.writeRegister(registers::ta,
                        duration_to_divider(times[0], clock_freq_));
  device_.writeRegister(registers::tb,
                        duration_to_divider(times[1], clock_freq_));
  device_.writeRegister(registers::te,
                        duration_to_divider(times[2], clock_freq_));
  device_.writeRegister(registers::th,
                        duration_to_divider(times[3], clock_freq_));
  device_.writeRegister(registers::vcc_offset,
                        duration_to_divider(times[4], clock_freq_));
  device_.writeRegister(registers::clk_offset,
                        duration_to_divider(times[5], clock_freq_));

  // TODO return correct times
  return times;
}

std::vector<std::byte> master::receive() {
  auto rx_level = rx_fifo_available();

  std::vector<std::byte> ret;
  ret.resize(rx_level);
  device_.readStream(registers::rx_fifo, gsl::span<std::byte>(ret));
  return ret;
}

bool master::receive(gsl::span<std::byte> buffer, duration timeout) {
  const auto end = std::chrono::system_clock::now() + timeout;
  while (rx_fifo_available() < buffer.size()) {
    if (std::chrono::system_clock::now() > end)
      return false;
  }

  device_.readStream(registers::rx_fifo, buffer);
  return true;
}

std::vector<std::byte>
master::receive(std::size_t n,
                duration timeout /* = std::chrono::seconds(1) */) {
  const auto end = std::chrono::system_clock::now() + timeout;
  while (rx_fifo_available() < n) {
    if (std::chrono::system_clock::now() > end)
      return {};
  }

  std::vector<std::byte> ret;
  ret.resize(n);
  device_.readStream(registers::rx_fifo, gsl::span<std::byte>(ret));
  return ret;
}

template <std::size_t N>
std::array<std::byte, N>
master::receive(duration timeout /* = std::chrono::seconds(1) */) {
  std::array<std::byte, N> ret;
  (void)receive(ret, timeout);
  return ret;
  // TODO what to do in error case
}

bool master::character_repetition() {
  auto reg = device_.readRegister(registers::config);
  return (reg & registers::config_charrep) != 0;
}

void master::set_character_repetition(bool charrep) {
  device_.readModifyWrite(registers::config, registers::config_charrep,
                          charrep ? registers::config_charrep : 0);
}

uint32_t master::character_guard_time() {
  auto reg = device_.readRegister(registers::config);
  return (reg & registers::config_cgt_msk) >> registers::config_cgt_pos;
}

void master::set_character_guard_time(uint32_t cgt) {
  if (((uint64_t)cgt << registers::config_cgt_pos) > registers::config_cgt_msk)
    throw std::runtime_error("cgt too big for available bits in register");
  device_.readModifyWrite(registers::config, registers::config_cgt_msk,
                          cgt << registers::config_cgt_pos);
}

master::duration master::iso_clock() const {
  auto reg = device_.readRegister(registers::clockrate); // TODO rename
  return divider_to_duration(reg, this->clock_freq_);
}

static uint32_t frequency_to_divider(master::frequency f,
                                     master::frequency module_clock) {
  auto exact_div = static_cast<double>(module_clock) / f;
  return static_cast<uint32_t>(std::lround(exact_div));
}

master::duration master::set_iso_clock(frequency f) {
  auto div = frequency_to_divider(f, clock_freq_);
  device_.writeRegister(registers::clockrate, div);
  return divider_to_duration(div, clock_freq_);
}

master::duration master::datarate() const {
  auto reg = device_.readRegister(registers::config2);
  return divider_to_duration(reg, clock_freq_);
}

master::duration master::set_datarate(frequency dr) {
  auto div = frequency_to_divider(dr, clock_freq_);
  device_.writeRegister(registers::config2, div);
  return divider_to_duration(div, clock_freq_);
}

master::duration master::block_timeout() const {
  return divider_to_duration(device_.readRegister(registers::config7),
                             clock_freq_);
}

master::duration master::set_block_timeout(duration timeout) {
  auto val = duration_to_divider(timeout, clock_freq_);
  device_.writeRegister(registers::config7, val);
  return divider_to_duration(val, clock_freq_);
}

void master::disable_block_timeout() {
  device_.writeRegister(registers::config7, 0);
}

master::duration master::character_timeout() const {
  return divider_to_duration(device_.readRegister(registers::config8),
                             clock_freq_);
}

master::duration master::set_character_timeout(duration timeout) {
  auto val = duration_to_divider(timeout, clock_freq_);
  device_.writeRegister(registers::config8, val);
  return divider_to_duration(val, clock_freq_);
}

void master::disable_character_timeout() {
  device_.writeRegister(registers::config8, 0);
}

uint16_t master::rx_fifo_available() const {
  auto ret = device_.readRegister(registers::buffers);
  return (ret & registers::buffers_rx_occupancy_msk) >>
         registers::buffers_rx_occupancy_pos;
}

uint16_t master::tx_fifo_free() const {
  auto ret = device_.readRegister(registers::buffers);
  return (ret & registers::buffers_tx_available_msk) >>
         registers::buffers_tx_available_pos;
}

} // namespace spinaltap::iso7816
