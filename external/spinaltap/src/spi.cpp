#include "spinaltap/spi/spi.hpp"
#include "spinaltap/spi/registers.hpp"
#include "spinaltap/logging.hpp"

namespace spinaltap::spi {

static uint32_t calc_divider(double freq, double module_freq,
                             uint32_t divider_width) {
  uint32_t divider = static_cast<double>(module_freq) / freq;
  if (divider < 1)
    throw std::runtime_error("invalid frequency, too high");
  if (divider >= (1 << divider_width))
    throw std::runtime_error("invalid frequency, too low");
  return divider;
}

master::master(device &device, uint32_t base_address)
    : device_(device), base_address_(base_address) {
  module_frequency_ = device.readRegister(base_address_ + registers::frequency);
  divider_width_ =
      device.readRegister(base_address_ + registers::prescaler_width);
  device.writeRegister(base_address_ + registers::trigger,
                       registers::trigger_flush_rx |
                           registers::trigger_flush_tx);
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
  return module_frequency_ / divider;
}

// TODO clearer logging for recv
void master::transceive(gsl::span<const uint8_t> tx, gsl::span<uint8_t> rx) {
  logging::logger->debug("SPI txrx: > {:n}", spdlog::to_hex(tx.begin(), tx.end()));

  if (rx.size() < tx.size())
    throw std::runtime_error("invalid buffer sizes");
  device_.writeStream(base_address_ + registers::tx, gsl::as_bytes(tx));
  device_.writeRegister(base_address_ + registers::trigger,
                        registers::trigger_flush_rx | registers::trigger_start);
  device_.poll(base_address_ + registers::status, registers::status_busy, 0);
  device_.readStream(base_address_ + registers::rx, gsl::as_writable_bytes(rx));

  logging::logger->debug("SPI txrx: < {:n}", spdlog::to_hex(rx.begin(), rx.end()));
}

void master::send(gsl::span<const uint8_t> tx) {
  logging::logger->debug("SPI txrx: > {:n}", spdlog::to_hex(tx.begin(), tx.end()));

  device_.writeStream(base_address_ + registers::tx, gsl::as_bytes(tx));
  device_.writeRegister(base_address_ + registers::trigger,
                        registers::trigger_flush_rx | registers::trigger_start);
  device_.poll(base_address_ + registers::status, registers::status_busy, 0);
}

void master::recv(gsl::span<uint8_t> rx) {
  std::vector<uint8_t> dummy(0, rx.size());
  transceive(dummy, rx);
}

} // namespace spinaltap::spi
