#ifndef spinaltap_util_h
#define spinaltap_util_h

#include <cstring>
#include <gsl/span>

namespace spinaltap::util {
void abort(gsl::span<const std::byte> dest, gsl::span<const std::byte> src);

template <typename T> void copy(gsl::span<T> dest, gsl::span<const T> src) {
  if (dest.size() < src.size())
    abort(gsl::as_bytes(dest), gsl::as_bytes(src));
  memcpy(dest.data(), src.data(), src.size());
}
} // namespace spinaltap::util

#endif
