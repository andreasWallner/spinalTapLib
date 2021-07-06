#ifndef spinaltap_gpio_registers_h
#define spinaltap_gpio_registers_h

#include <cstdint>

namespace spinaltap::gpio::registers {
    constexpr uint32_t read = 0x00U;
    constexpr uint32_t write = 0x04U;
    constexpr uint32_t write_enable = 0x08U;
}

#endif