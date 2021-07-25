#ifndef spinaltap_spi_registers_h
#define spinaltap_spi_registers_h

#include <cstdint>

namespace spinaltap::spi::registers {
constexpr uint32_t info = 0x0000U;
constexpr uint32_t frequency = 0x0004U;

constexpr uint32_t buffers = 0x0008U;
constexpr uint32_t buffers_rx_size_pos = 0;
constexpr uint32_t buffers_rx_size_msk = 0x00ffU;
constexpr uint32_t buffers_tx_size_pos = 16;
constexpr uint32_t buffers_tx_size_msk = 0xff00U;

constexpr uint32_t prescaler_width = 0x000cU;

constexpr uint32_t status = 0x0010U;
constexpr uint32_t status_busy = 0x0001U;
constexpr uint32_t status_rx_ovfl = 0x0002U;
constexpr uint32_t status_rx_occupancy_pos = 12;
constexpr uint32_t status_rx_occupancy_msk = 0x3ffUL << status_rx_occupancy_pos;
constexpr uint32_t status_tx_occupancy_pos = 22;
constexpr uint32_t status_tx_occupancy_msk = 0x3ffUL << status_tx_occupancy_pos;

constexpr uint32_t config = 0x0018U;
constexpr uint32_t config_cpha = 0x0001U;
constexpr uint32_t config_cpha_pos = 0;
constexpr uint32_t config_cpol = 0x0002U;
constexpr uint32_t config_cpol_pos = 1;
constexpr uint32_t config_bitorder_pos = 2;
constexpr uint32_t config_ss_pos = 3;
constexpr uint32_t config_prescaler_pos = 4;
constexpr uint32_t config_prescaler_msk = 0xfffff0U;

constexpr uint32_t trigger = 0x001cU;
constexpr uint32_t trigger_assert = 0x0001U;
constexpr uint32_t trigger_transceive = 0x0002U;
constexpr uint32_t trigger_deassert = 0x0004U;
constexpr uint32_t trigger_flush = 0x0008U;

constexpr uint32_t rx = 0x0020;
constexpr uint32_t tx = 0x0024;

constexpr uint32_t guard_times = 0x28;
constexpr uint32_t guard_times_word_pos = 0;
constexpr uint32_t guard_times_word_msk = 0xffU;
constexpr uint32_t guard_times_assert_pos = 8;
constexpr uint32_t guard_times_assert_msk = 0xff00U;
constexpr uint32_t guard_times_deassert_pos = 16;
constexpr uint32_t guard_times_deassert_msk = 0xff00000U;
} // namespace spinaltap::spi::registers

#endif
