#ifndef spinaltap_iomux_iomux_h
#define spinaltap_iomux_iomux_h

#include "spinaltap.hpp"

namespace spinaltap::iomux {

class iomux {
private:
  device &device_;
  uint32_t base_address_;

public:
  iomux(device &device, uint32_t base_address)
      : device_(device), base_address_(base_address) {}

  void connect(uint8_t input, uint8_t output);
};

} // namespace spinaltap::iomux

#endif