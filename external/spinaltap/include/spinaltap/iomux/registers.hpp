#ifndef spinaltap_iomux_registers_h
#define spinaltap_iomux_registers_h

namespace spinaltap::iomux::registers {
    constexpr uint32_t route(uint8_t id) noexcept {
        return id / 2 * 4;
    }
    constexpr uint32_t route_sel_pos(uint8_t id) noexcept {
        return (id & 1) ? 16 : 0;
    }
    constexpr uint32_t route_sel_msk(uint8_t id) noexcept {
        return 0xffff << route_sel_pos(id);
    }
}

#endif