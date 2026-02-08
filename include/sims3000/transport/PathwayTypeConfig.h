/**
 * @file PathwayTypeConfig.h
 * @brief Pathway type base stats definition for Epic 7 (Ticket E7-040)
 *
 * Defines tunable base stats per pathway type:
 * - Base capacity, build cost, decay rate multiplier, maintenance cost
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include "TransportEnums.h"

namespace sims3000 {
namespace transport {

/**
 * @struct PathwayTypeStats
 * @brief Static base stats for a pathway type.
 *
 * Each pathway type has fixed base stats that determine capacity,
 * cost, and decay characteristics. Runtime values are tracked
 * separately in ECS components.
 */
struct PathwayTypeStats {
    PathwayType type;
    uint16_t base_capacity;    ///< Maximum beings per tick
    uint16_t build_cost;       ///< Credits to construct
    uint8_t decay_rate_mult;   ///< Multiplier (100 = 1.0x, 75 = 0.75x)
    uint16_t maintenance_cost; ///< Credits per cycle
};

/**
 * @brief Get the default base stats for a given pathway type.
 *
 * @param type The PathwayType to query.
 * @return PathwayTypeStats with default values for that type.
 */
inline PathwayTypeStats get_pathway_stats(PathwayType type) {
    switch (type) {
        case PathwayType::BasicPathway:    return {type, 100, 50,  100, 2};
        case PathwayType::TransitCorridor: return {type, 500, 200, 50,  8};
        case PathwayType::Pedestrian:      return {type, 50,  25,  100, 1};
        case PathwayType::Bridge:          return {type, 200, 300, 75,  6};
        case PathwayType::Tunnel:          return {type, 200, 400, 60,  8};
        default: return {PathwayType::BasicPathway, 100, 50, 100, 2};
    }
}

} // namespace transport
} // namespace sims3000
