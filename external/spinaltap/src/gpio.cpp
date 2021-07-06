#include "spinaltap/gpio/gpio.hpp"
#include "spinaltap/gpio/registers.hpp"

namespace spinaltap::gpio {
uint32_t gpio::read() const { return device_.readRegister(registers::read); }

uint32_t gpio::write() const {
  return device_.readRegister(base_address_ + registers::write);
}

void gpio::set_write(uint32_t bits) {
  device_.writeRegister(base_address_ + registers::write, bits);
}

uint32_t gpio::write_enable() const {
  return device_.readRegister(base_address_ + registers::write_enable);
}

void gpio::set_write_enable(uint32_t bits) {
  device_.writeRegister(base_address_ + registers::write_enable, bits);
}
} // namespace spinaltap::gpio