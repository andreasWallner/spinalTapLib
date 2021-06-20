#pragma once

namespace spinaltap::iso7816::registers {

constexpr uint32_t frequency = 0x00U;
constexpr uint32_t buffer_sizes = 0x04U;

enum class status_state_t {
  inactive = 0,
  active = 1,
  reset = 2,
  clockstop = 3
};
constexpr uint32_t status = 0x08U;
constexpr uint32_t status_rx_active = 0x01U;
constexpr uint32_t status_tx_active = 0x02U;
constexpr uint32_t status_change_active = 0x04U;
constexpr uint32_t status_state_pos = 3;
constexpr uint32_t status_state_mask = 0x18U;
constexpr uint32_t status_rx_fifo_ovfl = 0x100U;
constexpr uint32_t status_tx_fifo_stall = 0x200U;

constexpr uint32_t config = 0x0CU;
constexpr uint32_t config_charrep = 0x01U;
constexpr uint32_t config_cgt_pos = 1;
constexpr uint32_t config_cgt_msk = 0x1EU;

constexpr uint32_t trigger = 0x10U;
constexpr uint32_t trigger_rx = 0x01U;
constexpr uint32_t trigger_tx = 0x02U;
constexpr uint32_t trigger_reset = 0x04U;
constexpr uint32_t trigger_deactivate = 0x08U;
constexpr uint32_t trigger_activate = 0x10U;
constexpr uint32_t trigger_stop_clock = 0x20U;
constexpr uint32_t trigger_warm_reset = trigger_reset | trigger_activate;
constexpr uint32_t trigger_cold_reset = trigger_deactivate | trigger_activate;
constexpr uint32_t trigger_rx_flush = 0x40U;
constexpr uint32_t trigger_tx_flush = 0x80U;

constexpr uint32_t clockrate = 0x14U;
constexpr uint32_t ta = 0x18U;
constexpr uint32_t tb = 0x1CU;
constexpr uint32_t te = 0x20U;
constexpr uint32_t th = 0x24U;
constexpr uint32_t vcc_offset = 0x28U;
constexpr uint32_t clk_offset = 0x2CU;
constexpr uint32_t fifo_levels = 0x30U;
constexpr uint32_t fifo_levels_rx_occupancy_pos = 0;
constexpr uint32_t fifo_levels_rx_occupancy_msk = 0xffffU;
constexpr uint32_t fifo_levels_tx_available_pos = 16;
constexpr uint32_t fifo_levels_tx_available_msk = 0xffff0000U;
constexpr uint32_t rx_fifo = 0x34U;
constexpr uint32_t tx_fifo = 0x38U;

} // namespace spinaltap::iso7816::registers
