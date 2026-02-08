/**
 * @file RailConfig.h
 * @brief Rail capacity and costs configuration for Epic 7 (Ticket E7-047)
 *
 * Defines tunable stats for rail types and terminal stations:
 * - RailTypeStats: Capacity, build cost, power requirements per rail type
 * - TerminalStats: Capacity, build cost, power for station types
 * - Rail cycle timing constant
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include "RailComponent.h"

namespace sims3000 {
namespace transport {

/**
 * @struct RailTypeStats
 * @brief Static configuration for a rail type.
 */
struct RailTypeStats {
    RailType type;
    uint16_t capacity;        ///< Beings per cycle
    uint16_t build_cost;      ///< Credits to construct
    uint16_t power_required;  ///< Energy units consumed
};

/**
 * @brief Get the default stats for a given rail type.
 *
 * @param type The RailType to query.
 * @return RailTypeStats with default values for that type.
 */
inline RailTypeStats get_rail_stats(RailType type) {
    switch (type) {
        case RailType::SurfaceRail:  return {type, 500, 200, 50};
        case RailType::ElevatedRail: return {type, 500, 350, 75};
        case RailType::SubterraRail: return {type, 750, 500, 100};
        default: return {RailType::SurfaceRail, 500, 200, 50};
    }
}

/**
 * @struct TerminalStats
 * @brief Static configuration for a terminal station.
 */
struct TerminalStats {
    uint16_t capacity;        ///< Beings capacity
    uint16_t build_cost;      ///< Credits to construct
    uint16_t power_required;  ///< Energy units consumed
};

/**
 * @brief Get stats for a surface station terminal.
 * @return TerminalStats with default surface station values.
 */
inline TerminalStats get_surface_station_stats()  { return {200, 300, 100}; }

/**
 * @brief Get stats for a subterra station terminal.
 * @return TerminalStats with default subterra station values.
 */
inline TerminalStats get_subterra_station_stats() { return {400, 500, 150}; }

/// Rail cycle length in ticks (5 seconds at 20 ticks/sec)
constexpr uint16_t RAIL_CYCLE_TICKS = 100;

} // namespace transport
} // namespace sims3000
