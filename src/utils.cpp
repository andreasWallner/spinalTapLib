#include "utils.hpp"
#include "fmt/core.h"

namespace utils {

static void dump_line(gsl::span<const uint8_t> bytes, uintptr_t addr,
                      uintptr_t empty_values = 0) {
  fmt::print("{:#010x} ", addr);
  for (int i = 0; i < empty_values; i++)
    fmt::print("   ");
  for (const auto b : bytes)
    fmt::print(" {:02x}", b);
  fmt::print("\n");
}

void hexdump(gsl::span<const uint8_t> bytes, int width /*=16*/,
             uintptr_t address /*=0*/) {
  uintptr_t frst_line_padding = address % width;
  uintptr_t line_addr = address - frst_line_padding;
  if (frst_line_padding != 0) {
    size_t cnt_first_line = std::min(width - frst_line_padding, bytes.size());
    dump_line(bytes.subspan(0, cnt_first_line), line_addr, frst_line_padding);
    bytes = bytes.subspan(cnt_first_line);
    line_addr += width;
  }
  in_chunks(bytes, width, [&](gsl::span<const uint8_t> s) {
    dump_line(s, line_addr);
    line_addr += width;
  });
}

} // namespace utils