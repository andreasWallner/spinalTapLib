#pragma once

#include <limits>
#include <string_view>

namespace numeric_utils {

class bad_lexical_cast : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace {

[[nodiscard]] constexpr int max_digit_char(int radix) noexcept {
  return radix <= 10 ? '0' + radix - 1 : '9';
}

template <typename T, int radix>[[nodiscard]] constexpr int max_safe_val() noexcept{
  static_assert(radix >= 2, "can't use radix < 2");
  constexpr int max{std::numeric_limits<T>::max()};
  constexpr int safe{max / radix};
  return safe;
}

template <typename T>
[[nodiscard]] constexpr T as_integer_16(const std::string_view s,
                                        bool &ok) noexcept {
  T result = 0;
  uint8_t overflow = 0;
  ok = true;
  for (const auto &c : s) {
    overflow |= result >> (sizeof(T) * 8 - 4);
    if (c >= '0' && c <= '9')
      result = (result << 4) + (c - '0');
    else if (c >= 'a' && c <= 'f')
      result = (result << 4) + (c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
      result = (result << 4) + (c - 'A' + 10);
    else
      ok = false;
  }
  ok = ok && overflow == 0;
  return result;
}

} // namespace

template <typename T, int radix>
[[nodiscard]] constexpr T as_integer(const std::string_view s,
                                     bool &ok) noexcept {
  static_assert(std::numeric_limits<T>::is_integer,
                "may only use lexical_cast with integer types");

  if (radix == 16)
    return as_integer_16<T>(s, ok);

  T result{0};
  T overflow{0};
  constexpr T max_safe{max_safe_val<T, radix>()};
  ok = true;
  for (const auto &c : s) {
    T digit_val{0};
    if (c >= '0' && c <= max_digit_char(radix))
      digit_val = c - '0';
    else if (radix > 10 && c >= 'a' && c <= ('a' + radix - 10))
      digit_val = c - 'a' + 10;
    else if (radix > 10 && c >= 'A' && c <= ('A' + radix - 10))
      digit_val = c - 'A' + 10;
    else
      ok = false;

    ok = ok && result < max_safe;

    result = result * radix + digit_val;
  }
  return result;
}

template <typename T, int radix>
[[nodiscard]] constexpr T as_integer(const std::string_view s) {
  bool ok = false;
  auto result = as_integer<T, radix>(s, ok);
  if (!ok)
    throw bad_lexical_cast("invalid hex string");
  return result;
}

template <typename T, int radix>
[[nodiscard]] bool is_number(std::string_view s) {
  bool ok;
  (void)as_integer<T, radix>(s, ok);
  return ok;
}

[[nodicard]] constexpr char flip_bits(char c) noexcept {
  c = (c & 0xF0) >> 4 | (c & 0x0F) << 4;
  c = (c & 0xCC) >> 2 | (c & 0x33) << 2;
  c = (c & 0xAA) >> 1 | (c & 0x55) << 1;
  return c;
}

} // namespace numeric_utils