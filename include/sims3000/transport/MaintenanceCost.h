/**
 * @file MaintenanceCost.h
 * @brief Per-owner maintenance cost calculation for Epic 7 (Ticket E7-021)
 *
 * Header-only utility for calculating maintenance costs based on pathway
 * type and current health. Cost scales with damage (missing health).
 *
 * Ownership is tracked externally; this utility only calculates the cost
 * for a given RoadComponent.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TransportEnums.h>
#include <sims3000/transport/RoadComponent.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @struct MaintenanceCostConfig
 * @brief Per-pathway-type maintenance cost rates.
 *
 * Each pathway type has a cost-per-health-point rate. The actual cost
 * for a pathway is proportional to missing health (damage).
 * Pedestrian pathways are free to maintain by default.
 */
struct MaintenanceCostConfig {
    uint16_t basic_cost_per_health    = 1;   ///< BasicPathway maintenance rate
    uint16_t corridor_cost_per_health = 3;   ///< TransitCorridor maintenance rate
    uint16_t pedestrian_cost_per_health = 0; ///< Pedestrian pathway (free)
    uint16_t bridge_cost_per_health   = 4;   ///< Bridge maintenance rate
    uint16_t tunnel_cost_per_health   = 4;   ///< Tunnel maintenance rate
};

/**
 * @brief Get the per-health maintenance cost rate for a pathway type.
 *
 * @param type The pathway type.
 * @param cfg Maintenance cost configuration.
 * @return Cost per health point for the given type.
 */
inline uint16_t cost_per_health(PathwayType type, const MaintenanceCostConfig& cfg = MaintenanceCostConfig{}) {
    switch (type) {
        case PathwayType::BasicPathway:    return cfg.basic_cost_per_health;
        case PathwayType::TransitCorridor: return cfg.corridor_cost_per_health;
        case PathwayType::Pedestrian:      return cfg.pedestrian_cost_per_health;
        case PathwayType::Bridge:          return cfg.bridge_cost_per_health;
        case PathwayType::Tunnel:          return cfg.tunnel_cost_per_health;
        default: return cfg.basic_cost_per_health;
    }
}

/**
 * @brief Calculate maintenance cost for a single road segment.
 *
 * Cost is proportional to missing health (255 - current health).
 * Formula: (missing_health * cost_per_health(type)) / 255
 *
 * A pristine pathway (health=255) costs 0.
 * A destroyed pathway (health=0) costs the full rate.
 *
 * @param road The road component to calculate cost for.
 * @param cfg Maintenance cost configuration.
 * @return Maintenance cost for this pathway segment.
 */
inline uint32_t calculate_maintenance_cost(const RoadComponent& road,
                                           const MaintenanceCostConfig& cfg = MaintenanceCostConfig{}) {
    uint32_t missing_health = 255 - road.health;
    return (missing_health * cost_per_health(road.type, cfg)) / 255;
}

} // namespace transport
} // namespace sims3000
