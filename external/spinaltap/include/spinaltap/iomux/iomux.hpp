#ifndef spinaltap_iomux_iomux_h
#define spinaltap_iomux_iomux_h

#include "spinaltap.hpp"

namespace spinaltap::iomux {

class iomux {
private:
  device &device_;
  uint32_t baseAddress_;

public:
  iomux(device &device) : device_(device) {}

  void connect(uint8_t output, uint8_t input);
};

} // namespace spinaltap::iomux

#endif