/**
 * @file EdgeCost.h
 * @brief Edge cost function for pathfinding (Epic 7, Ticket E7-024)
 *
 * Provides type-based edge cost calculation with congestion and decay penalties.
 * Used by the A* pathfinding system to weight pathway traversal costs.
 *
 * Cost = base_cost(type) + congestion_penalty + decay_penalty
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TransportEnums.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @struct EdgeCostConfig
 * @brief Tunable parameters for edge cost calculation.
 *
 * Defines base costs per pathway type and maximum penalty ranges
 * for congestion and decay factors.
 */
struct EdgeCostConfig {
    // Base costs per pathway type
    uint32_t basic_cost       = 15;   ///< BasicPathway base traversal cost
    uint32_t transit_cost     = 5;    ///< TransitCorridor base traversal cost
    uint32_t pedestrian_cost  = 20;   ///< Pedestrian pathway base traversal cost
    uint32_t bridge_cost      = 10;   ///< Bridge base traversal cost
    uint32_t tunnel_cost      = 10;   ///< Tunnel base traversal cost

    // Penalty ranges
    uint32_t max_congestion_penalty = 10;  ///< Maximum congestion penalty (0-10 from congestion_level)
    uint32_t max_decay_penalty      = 5;   ///< Maximum decay penalty (0-5 from missing health)
};

/**
 * @brief Calculate edge traversal cost for a pathway segment.
 *
 * Combines a type-based base cost with penalties for congestion
 * and pathway deterioration (missing health).
 *
 * @param type Pathway type determining base cost.
 * @param congestion_level Congestion severity (0-255).
 * @param health Pathway health/condition (0-255, 255=pristine).
 * @param config EdgeCostConfig with tunable base costs and penalty ranges.
 * @return Total edge cost (base + congestion_penalty + decay_penalty).
 */
inline uint32_t calculate_edge_cost(PathwayType type, uint8_t congestion_level,
                                    uint8_t health,
                                    const EdgeCostConfig& config = EdgeCostConfig{}) {
    uint32_t base = config.basic_cost;
    switch (type) {
        case PathwayType::BasicPathway:    base = config.basic_cost; break;
        case PathwayType::TransitCorridor: base = config.transit_cost; break;
        case PathwayType::Pedestrian:      base = config.pedestrian_cost; break;
        case PathwayType::Bridge:          base = config.bridge_cost; break;
        case PathwayType::Tunnel:          base = config.tunnel_cost; break;
    }

    // Congestion penalty: 0-max based on congestion_level (0-255)
    uint32_t congestion_penalty = (static_cast<uint32_t>(congestion_level) * config.max_congestion_penalty) / 255;

    // Decay penalty: 0-max based on missing health (255 - health)
    uint32_t missing_health = 255u - static_cast<uint32_t>(health);
    uint32_t decay_penalty = (missing_health * config.max_decay_penalty) / 255;

    return base + congestion_penalty + decay_penalty;
}

} // namespace transport
} // namespace sims3000
