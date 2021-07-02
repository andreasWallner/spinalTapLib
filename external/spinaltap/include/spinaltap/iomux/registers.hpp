#ifndef spinaltap_iomux_registers_h
#define spinaltap_iomux_registers_h

#include <cstdint>

namespace spinaltap::iomux::registers {
    constexpr uint32_t fields_per_register = 4;

    constexpr uint32_t route(uint8_t id) noexcept {
        return id / fields_per_register * 4;
    }
    constexpr uint32_t route_sel_pos(uint8_t id) noexcept {
        return (id % fields_per_register) ? (32 / 4) : 0;
    }
    constexpr uint32_t route_sel_msk(uint8_t id) noexcept {
        return 0xff << route_sel_pos(id); // TODO
    }
}

#endif