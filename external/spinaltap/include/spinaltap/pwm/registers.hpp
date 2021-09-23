#ifndef spinaltap_pwm_registers_h
#define spinaltap_pwm_registers_h

namespace spinaltap::pwm::registers {
constexpr uint32_t ctrl = 0x00U;
constexpr uint32_t ctrl_run = 0x01U;
constexpr uint32_t prescaler = 0x04U;
constexpr uint32_t max = 0x08U;
constexpr uint32_t level(uint8_t idx) noexcept { return static_cast<uint32_t>(0x0C + idx * 4); }
} // namespace spinaltap::pwm::registers

#endif
