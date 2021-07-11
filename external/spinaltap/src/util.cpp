#include "spinaltap/util.hpp"

#include <cstdlib>
#include <fmt/format.h>

namespace spinaltap::util {
void abort(gsl::span<const std::byte> dest, gsl::span<const std::byte> src) {
  fmt::print(stderr, "spinaltap abort handler was called in response to a "
                     "runtime violation\n");
  fmt::print(stderr, "copy failed since dst buffer was too small\n");
  fmt::print(stderr, "src: @0x{:x}, {}\n",
             reinterpret_cast<std::uintptr_t>(src.data()), src.size());
  fmt::print(stderr, "dst: @0x{:x}, {}\n\n",
             reinterpret_cast<std::uintptr_t>(dest.data()), dest.size());
  fmt::print(stderr,
             "Note to end users: This program was terminated as a result\n");
  fmt::print(stderr,
             "of a bug present in the software. Please reach out to your");
  fmt::print(stderr, "software's vendor to get more help.");

  std::abort();
}
} // namespace spinaltap::util
