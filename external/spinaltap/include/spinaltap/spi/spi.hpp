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
  uint32_t module_frequency_;
  int divider_width_;

public:
  master(device &device, uint32_t base_address);

  double configure(cpol pol, cpha pha, double frequency);
  void set_polarity(cpol pol);
  cpol polarity() const;
  void set_phase(cpha pha);
  cpha phase() const;
  double set_frequency(double frequency);
  double frequency() const;

  // inter-byte-wait
  // CS & CS wait

  void transceive(gsl::span<const uint8_t> tx, gsl::span<uint8_t> rx);
  void send(gsl::span<const uint8_t> tx);
  void recv(gsl::span<uint8_t> rx);
};

} // namespace spinaltap::spi

#endif
