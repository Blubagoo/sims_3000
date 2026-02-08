/**
 * @file CongestionCalculator.h
 * @brief Congestion calculation utilities for Epic 7 (Ticket E7-015)
 *
 * Provides:
 * - Congestion level calculation (0-255) based on flow vs capacity
 * - Blockage tick tracking for sustained congestion
 * - Contamination rate calculation for ContaminationSystem
 * - Sector value penalty lookup by congestion level
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TrafficComponent.h>
#include <sims3000/transport/RoadComponent.h>
#include <sims3000/transport/TrafficBalanceConfig.h>
#include <cstdint>
#include <algorithm>

namespace sims3000 {
namespace transport {

/**
 * @class CongestionCalculator
 * @brief Static utility class for congestion-related calculations.
 *
 * Calculates congestion level from flow/capacity ratio, updates
 * TrafficComponent fields, computes contamination rates, and
 * determines sector value penalties.
 */
class CongestionCalculator {
public:
    CongestionCalculator() = default;

    /**
     * @brief Calculate congestion level (0-255) for a pathway.
     *
     * Formula: (flow_current * 255) / max(current_capacity, 1), capped at 255.
     *
     * @param flow_current Current flow count on the pathway.
     * @param current_capacity Effective capacity of the pathway.
     * @return Congestion level 0-255 (0=free, 255=gridlock).
     */
    static uint8_t calculate_congestion(uint32_t flow_current, uint16_t current_capacity);

    /**
     * @brief Update TrafficComponent congestion fields from RoadComponent.
     *
     * Sets congestion_level and contamination_rate on the TrafficComponent
     * based on flow_current and the road's current_capacity.
     *
     * @param traffic TrafficComponent to update (modified in place).
     * @param road RoadComponent providing current_capacity.
     */
    static void update_congestion(TrafficComponent& traffic, const RoadComponent& road);

    /**
     * @brief Update flow_blockage_ticks counter for sustained congestion.
     *
     * Increments flow_blockage_ticks (capped at 255) when congestion_level
     * exceeds the threshold, resets to 0 when below.
     *
     * @param traffic TrafficComponent to update (modified in place).
     * @param threshold Congestion level above which blockage is counted (default: 200).
     */
    static void update_blockage_ticks(TrafficComponent& traffic, uint8_t threshold = 200);

    /**
     * @brief Calculate contamination rate for congested pathways.
     *
     * Only pathways with congestion > 128 produce contamination.
     * Formula: (congestion - 128) / 8, yielding range 0-15.
     *
     * @param congestion_level Current congestion level (0-255).
     * @return Contamination rate (0-15). 0 if congestion <= 128.
     */
    static uint8_t calculate_contamination_rate(uint8_t congestion_level);

    /**
     * @brief Get sector value penalty percentage for a congestion level.
     *
     * Returns the penalty percentage based on congestion thresholds
     * from TrafficBalanceConfig.
     *
     * @param congestion_level Current congestion level (0-255).
     * @param config TrafficBalanceConfig with threshold and penalty values.
     * @return Penalty percentage (0, 5, 10, or 15 by default).
     */
    static uint8_t get_penalty_percent(uint8_t congestion_level,
                                       const TrafficBalanceConfig& config = TrafficBalanceConfig{});
};

} // namespace transport
} // namespace sims3000
