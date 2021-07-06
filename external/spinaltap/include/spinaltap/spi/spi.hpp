#ifndef spinaltap_spi_spi_h
#define spinaltap_spi_spi_h

#include "spinaltap.hpp"
#include <chrono>

namespace spinaltap::spi {

enum class cpol { idle_low = 0, idle_high = 1 };
enum class cpha { first_edge_shifts = 0, first_edge_latches = 1 };

class master {
private:
  device &device_;
  uint32_t base_address_;

public:
  master(device &device, uint32_t base_address)
      : device_(device), base_address_(base_address) {}

  void configure(cpol pol, cpha pha, double frequency);
};

} // namespace spinaltap::spi

#endif