#ifndef spinaltap_gpio_gpio_h
#define spinaltap_gpio_gpio_h

#include "spinaltap.hpp"

namespace spinaltap::gpio {
class gpio {
private:
  device &device_;
  uint32_t base_address_;

public:
  gpio(device &device, uint32_t base_address)
      : device_(device), base_address_(base_address) {}

  uint32_t read() const;
  uint32_t write() const;
  void set_write(uint32_t bits);
  uint32_t write_enable() const;
  void set_write_enable(uint32_t bits);
};
} // namespace spinaltap::gpio

#endif