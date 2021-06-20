#pragma once

#include <limits>
#include <cstdint>

template <class UIntType, int a, int b, int c> class xorshift_engine {
public:
  using result_type = UIntType;
  static constexpr int shift1 = a;
  static constexpr int shift2 = b;
  static constexpr int shift3 = c;
  static constexpr result_type default_seed = 4521U;

  xorshift_engine() : xorshift_engine(default_seed) {}
  xorshift_engine(result_type seed) : state_(seed) {}

  void seed(result_type seed = default_seed) noexcept { state_ = seed; }

  [[nodiscard]] result_type operator()() noexcept {
    state_ ^= state_ << a;
    state_ ^= state_ >> b;
    state_ ^= state_ << c;
    return state_;
  }

  void discard(unsigned long long z) noexcept {
    while (z--)
      (void)operator()();
  }

  [[nodiscard]] static result_type min() noexcept { return 1; }
  [[nodiscard]] static result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

  [[nodiscard]] bool operator==(const xorshift_engine<result_type, a, b, c>& other) const noexcept {
    return state_ == other.state_;
  }

  [[nodiscard]] bool operator!=(const xorshift_engine<result_type, a, b, c>& other) const noexcept {
    return !(*this == other);
  }

private:
  result_type state_;
};

using xorshift16 = xorshift_engine<uint16_t, 7, 9, 8>;
using xorshift32 = xorshift_engine<uint32_t, 13, 17, 5>;
using xorshift64 = xorshift_engine<uint64_t, 13, 7, 17>;
