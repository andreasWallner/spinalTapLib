#pragma once

#include "spinaltap.hpp"

#include <chrono>

namespace spinaltap {

class pwm {
private:
  device &device_;
  uint32_t baseAddress_;

public:
  constexpr static unsigned long frequency_ = 100'000'000UL;

  pwm(device &device, uint32_t baseAddress)
      : device_(device), baseAddress_(baseAddress) {}

  uint32_t max_count() const;
  void set_max_count(uint32_t d);
  template<class _Rep, class _Period=std::ratio<1i64>>
  void set_period(std::chrono::duration<_Rep, _Period> period);

  uint8_t width(uint8_t idx) const;
  void set_width(uint8_t idx, uint8_t w);

  std::array<uint8_t, 3> widths() const;
  void set_widths(gsl::span<uint8_t, 3> w);
};

template <class _Rep, class _Period>
inline void pwm::set_period(std::chrono::duration<_Rep, _Period> period) {
  // TODO
}

} // namespace spinaltap
