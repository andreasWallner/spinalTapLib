#pragma once

#include <stdint.h>
#include "gsl/span"

namespace utils {

void hexdump(gsl::span<const uint8_t> bytes, int width = 16,
             uintptr_t address = 0);

// TODO rewrite using std::ranges
template <typename T, typename func_t>
void in_chunks(gsl::span<T> s, size_t chunk_size,
               func_t f) noexcept(noexcept(f)) {
  const size_t size = s.size();
  for (size_t offset = 0; offset < size; offset += chunk_size) {
    const size_t this_chunk_size = std::min(chunk_size, size - offset);
    f(s.subspan(offset, this_chunk_size));
  }
}
} // namespace utils