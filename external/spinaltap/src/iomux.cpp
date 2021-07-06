#include "spinaltap/iomux/iomux.hpp"
#include "spinaltap/iomux/registers.hpp"

namespace spinaltap::iomux {

void iomux::connect(uint8_t input, uint8_t output) {
  auto reg = device_.readRegister(base_address_ + registers::route(output));
  device_.writeRegister(base_address_ + registers::route(output),
                        reg & ~registers::route_sel_msk(output) |
                            (input << registers::route_sel_pos(output)));
}

} // namespace spinaltap::iomux