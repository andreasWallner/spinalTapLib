#include "pwm.hpp"
#include "registers.hpp"

namespace spinaltap {

static uint32_t idx_to_address(uint8_t idx) {
  if (idx > 2)
    throw std::runtime_error("invalid PWM index"); // TODO custom exception
  return registers::pwm::pwm0 + idx * sizeof(uint32_t);
}

uint32_t pwm::max_count() const {
  return device_.readRegister(registers::pwm::max_count);
}

void pwm::set_max_count(uint32_t d) {
  device_.writeRegister(registers::pwm::max_count, d);
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
  w[0] = device_.readRegister(registers::pwm::pwm0);
  w[1] = device_.readRegister(registers::pwm::pwm1);
  w[2] = device_.readRegister(registers::pwm::pwm2);
  return w;
}

void pwm::set_widths(gsl::span<uint8_t, 3> w) {
  device_.writeRegisters({{registers::pwm::pwm0, w[0]},
                          {registers::pwm::pwm1, w[1]},
                          {registers::pwm::pwm2, w[2]}});
}

} // namespace spinaltap
