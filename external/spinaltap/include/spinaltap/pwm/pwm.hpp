#ifndef spinaltap_pwm_pwm_h
#define spinaltap_pwm_pwm_h

#include "spinaltap.hpp"
#include <chrono>

namespace spinaltap::pwm {

class pwm {
private:
  device &device_;
  uint32_t baseAddress_;

public:
  constexpr static std::chrono::duration<std::chrono::system_clock::rep,
                                         std::ratio<1, 100'000'000>>
      clock_period_{1};

  pwm(device &device, uint32_t baseAddress)
      : device_(device), baseAddress_(baseAddress) {}

  uint32_t max_count() const;
  void set_max_count(uint32_t d);
  uint32_t prescaler() const;
  void set_prescaler(uint32_t pre);

  template <class _Rep, class _Period>
  std::chrono::duration<_Rep, _Period>
  set_period(std::chrono::duration<_Rep, _Period> period, uint32_t max_count);

  uint8_t width(uint8_t idx) const;
  void set_width(uint8_t idx, uint8_t w);

  std::array<uint8_t, 3> widths() const;
  void set_widths(gsl::span<uint8_t, 3> w);

private:
  uint32_t idx_to_address(int idx) const;
};

template <class _Rep, class _Period>
inline std::chrono::duration<_Rep, _Period>
pwm::set_period(std::chrono::duration<_Rep, _Period> period,
                uint32_t max_count) {
  auto clocks = period / clock_period_;
  // TODO after HW module changes... (add pre-divider)
}

} // namespace spinaltap::pwm

#endif