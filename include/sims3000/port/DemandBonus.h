/**
 * @file DemandBonus.h
 * @brief Global demand bonus calculation from operational ports (Epic 8, Ticket E8-016)
 *
 * Calculates colony-wide demand bonuses from operational port facilities:
 * - Aero ports boost Exchange zone demand
 * - Aqua ports boost Fabrication zone demand
 *
 * Port size thresholds determine bonus per port:
 * - Small  (capacity < 500):       +5
 * - Medium (capacity 500-1999):    +10
 * - Large  (capacity >= 2000):     +15
 *
 * Maximum total port demand bonus is capped at +30.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Port size threshold: Small < 500
constexpr uint16_t PORT_SIZE_SMALL_MAX = 499;

/// Port size threshold: Medium 500-1999
constexpr uint16_t PORT_SIZE_MEDIUM_MIN = 500;
constexpr uint16_t PORT_SIZE_MEDIUM_MAX = 1999;

/// Port size threshold: Large >= 2000
constexpr uint16_t PORT_SIZE_LARGE_MIN = 2000;

/// Demand bonus for Small ports
constexpr float DEMAND_BONUS_SMALL = 5.0f;

/// Demand bonus for Medium ports
constexpr float DEMAND_BONUS_MEDIUM = 10.0f;

/// Demand bonus for Large ports
constexpr float DEMAND_BONUS_LARGE = 15.0f;

/// Maximum total port demand bonus (prevents infinite stacking)
constexpr float MAX_TOTAL_DEMAND_BONUS = 30.0f;

/// Local demand bonus: Aero ports boost Habitation within this radius
constexpr int32_t LOCAL_BONUS_AERO_RADIUS = 20;

/// Local demand bonus: Aqua ports boost Exchange within this radius
constexpr int32_t LOCAL_BONUS_AQUA_RADIUS = 25;

/// Local demand bonus: Aero port Habitation bonus percentage
constexpr float LOCAL_BONUS_AERO_HABITATION = 5.0f;

/// Local demand bonus: Aqua port Exchange bonus percentage
constexpr float LOCAL_BONUS_AQUA_EXCHANGE = 10.0f;

/**
 * @struct PortData
 * @brief Minimal port data needed for demand bonus calculation.
 *
 * Contains the fields required by calculate_global_demand_bonus and
 * calculate_local_demand_bonus. Position fields (x, y) default to 0
 * for backward compatibility with global-only calculations.
 */
struct PortData {
    PortType port_type;         ///< Aero or Aqua
    uint16_t capacity;          ///< Current capacity
    bool is_operational;        ///< Whether the port is operational
    uint8_t owner;              ///< Owner player ID
    int32_t x = 0;             ///< Port X position (tile coordinates)
    int32_t y = 0;             ///< Port Y position (tile coordinates)
};

/**
 * @brief Get the demand bonus contribution for a single port based on its capacity.
 *
 * @param capacity The port's current capacity.
 * @return Bonus value: 5.0 (Small), 10.0 (Medium), or 15.0 (Large). Returns 0 if capacity is 0.
 */
float get_port_size_bonus(uint16_t capacity);

/**
 * @brief Calculate the global demand bonus for a specific zone type from operational ports.
 *
 * Iterates all ports matching the owner. Aero ports contribute to Exchange demand,
 * Aqua ports contribute to Fabrication demand. Only operational ports contribute.
 *
 * The zone_type parameter determines which port type is checked:
 * - zone_type 1 (Exchange):    sums bonuses from Aero ports
 * - zone_type 2 (Fabrication): sums bonuses from Aqua ports
 * - Other zone types:          returns 0.0
 *
 * Result is capped at MAX_TOTAL_DEMAND_BONUS (30.0).
 *
 * @param zone_type The zone type to calculate bonus for (1=Exchange, 2=Fabrication).
 * @param owner Owner player ID to filter ports.
 * @param ports Vector of all port data.
 * @return Demand bonus (0.0 to MAX_TOTAL_DEMAND_BONUS).
 */
float calculate_global_demand_bonus(uint8_t zone_type, uint8_t owner,
                                     const std::vector<PortData>& ports);

/**
 * @brief Calculate the local (radius-based) demand bonus at a specific position.
 *
 * Checks all ports matching the owner for proximity-based demand bonuses:
 * - Aero ports within 20-tile Manhattan distance boost Habitation (zone_type 0) by +5%
 * - Aqua ports within 25-tile Manhattan distance boost Exchange (zone_type 1) by +10%
 *
 * Multiple ports' local bonuses stack. The returned value is the raw local bonus;
 * callers should cap the combined (global + local) total at MAX_TOTAL_DEMAND_BONUS.
 *
 * Only operational ports contribute. Non-operational ports are ignored.
 *
 * @param zone_type The zone type at the query position (0=Habitation, 1=Exchange).
 * @param x X coordinate of the query position (tile coordinates).
 * @param y Y coordinate of the query position (tile coordinates).
 * @param owner Owner player ID to filter ports.
 * @param ports Vector of all port data (with positions).
 * @return Local demand bonus (0.0 or positive). Not independently capped.
 */
float calculate_local_demand_bonus(uint8_t zone_type, int32_t x, int32_t y,
                                     uint8_t owner,
                                     const std::vector<PortData>& ports);

/**
 * @brief Calculate combined (global + local) demand bonus, capped at MAX_TOTAL_DEMAND_BONUS.
 *
 * Convenience function that sums global and local bonuses and applies the +30 cap.
 *
 * @param zone_type The zone type (0=Habitation, 1=Exchange, 2=Fabrication).
 * @param x X coordinate of the query position (tile coordinates).
 * @param y Y coordinate of the query position (tile coordinates).
 * @param owner Owner player ID to filter ports.
 * @param ports Vector of all port data.
 * @return Combined demand bonus (0.0 to MAX_TOTAL_DEMAND_BONUS).
 */
float calculate_combined_demand_bonus(uint8_t zone_type, int32_t x, int32_t y,
                                        uint8_t owner,
                                        const std::vector<PortData>& ports);

} // namespace port
} // namespace sims3000
