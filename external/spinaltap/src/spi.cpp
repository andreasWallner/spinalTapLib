#include "spinaltap/spi/spi.hpp"
#include "spinaltap/logging.hpp"
#include "spinaltap/spi/registers.hpp"

namespace spinaltap::spi {

constexpr static uint32_t calc_divider(double freq, double module_freq,
                                       uint32_t divider_width) {
  uint32_t divider = static_cast<uint32_t>(module_freq / freq);
  if (divider < 1)
    throw std::runtime_error("invalid frequency, too high");
  if (divider >= (1U << divider_width))
    throw std::runtime_error("invalid frequency, too low");
  return divider;
}

static double calc_frequency(uint32_t divider, double module_freq,
                             uint32_t divider_width) {
  return module_freq / static_cast<double>(divider);
}

constexpr static uint32_t ss_to_trigger(ss_action ss) noexcept {
  uint32_t trigger = 0;
  if (ss == ss_action::assert || ss == ss_action::both)
    trigger += registers::trigger_assert;
  if (ss == ss_action::deassert || ss == ss_action::both)
    trigger += registers::trigger_deassert;
  return trigger;
}

master::master(device &device, uint32_t base_address)
    : device_(device), base_address_(base_address) {
  module_frequency_ = device.readRegister(base_address_ + registers::frequency);
  divider_width_ =
      device.readRegister(base_address_ + registers::prescaler_width);
  device.writeRegister(base_address_ + registers::trigger,
                       registers::trigger_flush);
}

double master::configure(cpol pol, cpha pha, double frequency) {
  uint32_t divider = calc_divider(frequency, module_frequency_, divider_width_);
  uint32_t conf = (static_cast<uint32_t>(pol) << registers::config_cpol_pos) |
                  (static_cast<uint32_t>(pha) << registers::config_cpha_pos) |
                  (divider << registers::config_prescaler_pos);
  device_.writeRegister(base_address_ + registers::config, conf);
  return module_frequency_ / divider;
}

void master::set_polarity(cpol pol) {
  device_.readModifyWrite(
      base_address_ + registers::config, ~registers::config_cpol,
      static_cast<uint32_t>(pol) << registers::config_cpol_pos);
}

cpol master::polarity() const {
  uint32_t config = device_.readRegister(base_address_ + registers::config);
  return static_cast<cpol>((config & registers::config_cpol) >>
                           registers::config_cpol_pos);
}

void master::set_phase(cpha pha) {
  device_.readModifyWrite(
      base_address_ + registers::config, ~registers::config_cpha,
      static_cast<uint32_t>(pha) << registers::config_cpha_pos);
}

cpha master::phase() const {
  uint32_t config = device_.readRegister(base_address_ + registers::config);
  return static_cast<cpha>((config & registers::config_cpha) >>
                           registers::config_cpha_pos);
}

double master::set_frequency(double frequency) {
  uint32_t divider = calc_divider(frequency, module_frequency_, divider_width_);
  device_.readModifyWrite(base_address_ + registers::config,
                          registers::config_prescaler_msk,
                          divider << registers::config_prescaler_pos);
  return calc_frequency(divider, module_frequency_, divider_width_);
}

double master::frequency() const {
  uint32_t divider = (device_.readRegister(base_address_ + registers::config) &
                      registers::config_prescaler_msk) >>
                     registers::config_prescaler_pos;
  return calc_frequency(divider, module_frequency_, divider_width_);
}

uint8_t master::word_guard_clocks() const {
  uint8_t clocks = (device_.readRegister(registers::guard_times) &
                    registers::guard_times_word_msk) >>
                   registers::guard_times_word_pos;

  return clocks;
}

void master::set_word_guard_clocks(uint8_t clocks) {
  logging::logger->debug("SPI set {} word guard clocks", clocks);

  device_.readModifyWrite(registers::guard_times,
                          registers::guard_times_word_msk,
                          clocks << registers::guard_times_word_pos);
}

uint8_t master::ss_assert_guard_clocks() const {
  uint8_t clocks = (device_.readRegister(registers::guard_times) &
                    registers::guard_times_assert_msk) >>
                   registers::guard_times_assert_pos;

  return clocks;
}

void master::set_ss_assert_guard_clocks(uint8_t clocks) {
  logging::logger->debug("SPI set {} assert clocks", clocks);

  device_.readModifyWrite(registers::guard_times,
                          registers::guard_times_assert_msk,
                          clocks << registers::guard_times_assert_pos);
}

uint8_t master::ss_deassert_guard_clocks() const {
  auto clocks = (device_.readRegister(registers::guard_times) &
                 registers::guard_times_deassert_msk) >>
                registers::guard_times_deassert_pos;

  return static_cast<uint8_t>(clocks);
}

void master::set_ss_deassert_guard_clocks(uint8_t clocks) {
  logging::logger->debug("SPI set {} word guard clocks", clocks);

  device_.readModifyWrite(registers::guard_times,
                          registers::guard_times_deassert_msk,
                          clocks << registers::guard_times_deassert_pos);
}

void master::transceive(gsl::span<const uint8_t> tx, gsl::span<uint8_t> rx,
                        ss_action ss) {
  logging::logger->debug("SPI txrx: > {:n}",
                         spdlog::to_hex(tx.begin(), tx.end()));

  raw_transceive(tx, rx, ss);

  logging::logger->debug("SPI txrx: < {:n}",
                         spdlog::to_hex(rx.begin(), rx.end()));
}

void master::send(gsl::span<const uint8_t> tx, ss_action ss) {
  logging::logger->debug("SPI txrx: > {:n}",
                         spdlog::to_hex(tx.begin(), tx.end()));

  device_.writeStream(base_address_ + registers::tx, gsl::as_bytes(tx));
  device_.writeRegister(base_address_ + registers::trigger,
                        registers::trigger_flush |
                            registers::trigger_transceive | ss_to_trigger(ss));
  if (!device_.poll(base_address_ + registers::status, registers::status_busy,
                    0))
    throw std::runtime_error("timeout, poll never finished");
}

void master::recv(gsl::span<uint8_t> rx, ss_action ss) {
  logging::logger->debug("SPI rx {} bytes", rx.size());

  std::vector<uint8_t> dummy(rx.size(), 0);
  transceive(dummy, rx, ss);

  logging::logger->debug("SPI rx: < {:n}",
                         spdlog::to_hex(rx.begin(), rx.end()));
}

void master::raw_transceive(gsl::span<const uint8_t> tx, gsl::span<uint8_t> rx,
                            ss_action ss) {
  if (rx.size() < tx.size())
    throw std::runtime_error("invalid buffer sizes");

  device_.writeStream(base_address_ + registers::tx, gsl::as_bytes(tx));
  device_.writeRegister(base_address_ + registers::trigger,
                        registers::trigger_flush |
                            registers::trigger_transceive | ss_to_trigger(ss));
  if (!device_.poll(base_address_ + registers::status, registers::status_busy,
                    0))
    throw std::runtime_error("timeout, poll never finished");
  device_.readStream(base_address_ + registers::rx, gsl::as_writable_bytes(rx));
}

} // namespace spinaltap::spi
