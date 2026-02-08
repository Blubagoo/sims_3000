/**
 * @file DecayConfig.h
 * @brief Decay rates and thresholds configuration for Epic 7 (Ticket E7-046)
 *
 * Defines:
 * - DecayThresholds: Health thresholds for visual state mapping
 * - PathwayHealthState: Enum for health state categories
 * - DecayConfig: Tunable decay rate parameters
 *
 * Health is stored as uint8_t (0-255). Visual state is derived from
 * health value using configurable thresholds.
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @struct DecayThresholds
 * @brief Health thresholds for visual state mapping.
 *
 * Maps a uint8_t health value to one of five visual states:
 * - Pristine:  pristine_min - 255
 * - Good:      good_min - (pristine_min - 1)
 * - Worn:      worn_min - (good_min - 1)
 * - Poor:      poor_min - (worn_min - 1)
 * - Crumbling: 0 - (poor_min - 1)
 */
struct DecayThresholds {
    uint8_t pristine_min = 200;  ///< 200-255
    uint8_t good_min     = 150;  ///< 150-199
    uint8_t worn_min     = 100;  ///< 100-149
    uint8_t poor_min     = 50;   ///< 50-99
    // 0-49 = Crumbling
};

/**
 * @enum PathwayHealthState
 * @brief Visual/gameplay state derived from pathway health.
 */
enum class PathwayHealthState : uint8_t {
    Pristine  = 0,
    Good      = 1,
    Worn      = 2,
    Poor      = 3,
    Crumbling = 4
};

/**
 * @brief Determine the health state from a raw health value.
 *
 * @param health The current health value (0-255).
 * @param t The decay thresholds to use (defaults to standard thresholds).
 * @return PathwayHealthState corresponding to the health value.
 */
inline PathwayHealthState get_health_state(uint8_t health, const DecayThresholds& t = DecayThresholds{}) {
    if (health >= t.pristine_min) return PathwayHealthState::Pristine;
    if (health >= t.good_min)     return PathwayHealthState::Good;
    if (health >= t.worn_min)     return PathwayHealthState::Worn;
    if (health >= t.poor_min)     return PathwayHealthState::Poor;
    return PathwayHealthState::Crumbling;
}

/**
 * @struct DecayConfig
 * @brief Tunable decay rate parameters.
 *
 * Controls how quickly pathways deteriorate over time:
 * - base_decay_per_cycle: Health lost each decay cycle
 * - decay_cycle_ticks: Ticks between decay applications
 * - max_traffic_multiplier: Maximum decay multiplier from traffic
 */
struct DecayConfig {
    uint8_t base_decay_per_cycle = 1;    ///< Health lost per decay cycle
    uint16_t decay_cycle_ticks   = 100;  ///< Ticks between decay applications
    uint8_t max_traffic_multiplier = 3;  ///< Max 3x decay with max traffic
};

} // namespace transport
} // namespace sims3000
