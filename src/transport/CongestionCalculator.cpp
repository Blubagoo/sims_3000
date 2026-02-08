/**
 * @file CongestionCalculator.cpp
 * @brief Congestion calculation implementation (Epic 7, Ticket E7-015)
 *
 * Implements congestion level computation, blockage tracking,
 * contamination rate, and sector value penalty calculations.
 *
 * @see CongestionCalculator.h for class documentation.
 */

#include <sims3000/transport/CongestionCalculator.h>
#include <algorithm>

namespace sims3000 {
namespace transport {

// =============================================================================
// Congestion level calculation
// =============================================================================

uint8_t CongestionCalculator::calculate_congestion(uint32_t flow_current, uint16_t current_capacity) {
    uint32_t cap = (current_capacity > 0) ? static_cast<uint32_t>(current_capacity) : 1u;
    uint32_t raw = (flow_current * 255u) / cap;
    return static_cast<uint8_t>((std::min)(raw, 255u));
}

// =============================================================================
// Update TrafficComponent congestion fields
// =============================================================================

void CongestionCalculator::update_congestion(TrafficComponent& traffic, const RoadComponent& road) {
    traffic.congestion_level = calculate_congestion(traffic.flow_current, road.current_capacity);
    traffic.contamination_rate = calculate_contamination_rate(traffic.congestion_level);
}

// =============================================================================
// Blockage tick tracking
// =============================================================================

void CongestionCalculator::update_blockage_ticks(TrafficComponent& traffic, uint8_t threshold) {
    if (traffic.congestion_level > threshold) {
        if (traffic.flow_blockage_ticks < 255) {
            traffic.flow_blockage_ticks++;
        }
    } else {
        traffic.flow_blockage_ticks = 0;
    }
}

// =============================================================================
// Contamination rate
// =============================================================================

uint8_t CongestionCalculator::calculate_contamination_rate(uint8_t congestion_level) {
    if (congestion_level <= 128) {
        return 0;
    }
    return static_cast<uint8_t>((congestion_level - 128) / 8);
}

// =============================================================================
// Sector value penalty
// =============================================================================

uint8_t CongestionCalculator::get_penalty_percent(uint8_t congestion_level,
                                                   const TrafficBalanceConfig& config) {
    if (congestion_level <= config.free_flow_max) {
        return 0;
    }
    if (congestion_level <= config.light_max) {
        return config.light_penalty_pct;
    }
    if (congestion_level <= config.moderate_max) {
        return config.moderate_penalty_pct;
    }
    if (congestion_level <= config.heavy_max) {
        return config.heavy_penalty_pct;
    }
    // Blockage level (> heavy_max)
    return config.heavy_penalty_pct;
}

} // namespace transport
} // namespace sims3000
