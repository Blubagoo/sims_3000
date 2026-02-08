/**
 * @file TrafficBalanceConfig.h
 * @brief Traffic contribution balance values for Epic 7 (Ticket E7-048)
 *
 * Defines tunable traffic balance parameters:
 * - Per-building-type flow contribution rates
 * - Level multiplier for scaling flow with building level
 * - Congestion thresholds (0-255 scale)
 * - Sector value penalties per congestion level
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @struct TrafficBalanceConfig
 * @brief Tunable traffic contribution and congestion parameters.
 *
 * Controls how buildings generate traffic and how congestion
 * affects sector value. All values are tunable defaults.
 */
struct TrafficBalanceConfig {
    // Base flow per tick per building
    uint32_t habitation_flow  = 2;   ///< Flow from habitation buildings
    uint32_t exchange_flow    = 5;   ///< Flow from exchange buildings
    uint32_t fabrication_flow = 3;   ///< Flow from fabrication buildings

    // Level multiplier (level * base)
    uint8_t level_multiplier  = 1;   ///< Applied as level * multiplier * base

    // Congestion thresholds (0-255 scale)
    uint8_t free_flow_max     = 50;  ///< 0-50 = free flow
    uint8_t light_max         = 100; ///< 51-100 = light congestion
    uint8_t moderate_max      = 150; ///< 101-150 = moderate congestion
    uint8_t heavy_max         = 200; ///< 151-200 = heavy congestion
    // 201-255 = flow_blockage

    // Sector value penalties per congestion level
    uint8_t light_penalty_pct    = 5;   ///< -5% sector value
    uint8_t moderate_penalty_pct = 10;  ///< -10% sector value
    uint8_t heavy_penalty_pct    = 15;  ///< -15% sector value
};

} // namespace transport
} // namespace sims3000
