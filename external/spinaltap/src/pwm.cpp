#include "spinaltap/pwm/pwm.hpp"
#include "spinaltap/pwm/registers.hpp"

namespace spinaltap::pwm {

uint32_t pwm::idx_to_address(int idx) const {
  if (idx > 2)
    throw std::runtime_error("invalid PWM index"); // TODO custom exception
  return baseAddress_ + registers::level(static_cast<uint8_t>(idx));
}

uint32_t pwm::max_count() const {
  return device_.readRegister(baseAddress_ + registers::max);
}

void pwm::set_max_count(uint32_t d) {
  device_.writeRegister(baseAddress_ + registers::max, d);
}

uint32_t pwm::prescaler() const {
  return device_.readRegister(baseAddress_ + registers::prescaler);
}

void pwm::set_prescaler(uint32_t pre) {
  device_.writeRegister(baseAddress_ + registers::prescaler, pre);
}

uint8_t pwm::width(uint8_t idx) const {
  return static_cast<uint8_t>(device_.readRegister(idx_to_address(idx)));
}

void pwm::set_width(uint8_t idx, uint8_t w) {
  device_.writeRegister(idx_to_address(idx), w);
}

std::array<uint8_t, 3> pwm::widths() const {
  std::array<uint8_t, 3> w;
  // TODO read all at once
  for (int idx = 0; idx < w.size(); idx++)
    w[idx] = static_cast<uint8_t>(device_.readRegister(idx_to_address(idx)));
  return w;
}

void pwm::set_widths(gsl::span<uint8_t, 3> w) {
  device_.writeRegisters({{idx_to_address(0), w[0]},
                          {idx_to_address(1), w[1]},
                          {idx_to_address(2), w[2]}});
}

} // namespace spinaltap::pwm
