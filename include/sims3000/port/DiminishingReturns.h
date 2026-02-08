/**
 * @file DiminishingReturns.h
 * @brief Diminishing returns for multiple ports of the same type (Epic 8, Ticket E8-035)
 *
 * Implements diminishing returns when a player builds multiple ports
 * of the same type:
 *
 * - 1st port: 100% of base bonus
 * - 2nd port: 50% of base bonus
 * - 3rd port: 25% of base bonus
 * - 4th+ port: 12.5% of base bonus (halves each time)
 *
 * This encourages diversifying port types rather than stacking
 * multiple copies of the same port type.
 *
 * Depends on: E8-016 (DemandBonus.h) for PortData and bonus calculation.
 *
 * Header-only implementation (pure logic, no external dependencies beyond DemandBonus.h).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace sims3000 {
namespace port {

/// Diminishing returns multiplier for the first port (100%)
constexpr float DIMINISHING_FIRST = 1.0f;

/// Diminishing returns multiplier for the second port (50%)
constexpr float DIMINISHING_SECOND = 0.5f;

/// Diminishing returns multiplier for the third port (25%)
constexpr float DIMINISHING_THIRD = 0.25f;

/// Minimum diminishing returns multiplier (floor to prevent near-zero contributions)
constexpr float DIMINISHING_FLOOR = 0.125f;

/**
 * @brief Calculate the diminishing returns multiplier for a port by index.
 *
 * Returns the multiplier for the Nth port of the same type (0-indexed):
 * - Index 0 (1st port): 1.0
 * - Index 1 (2nd port): 0.5
 * - Index 2 (3rd port): 0.25
 * - Index 3+ (4th+ port): 0.125 (floor)
 *
 * The pattern halves each time: 1.0 -> 0.5 -> 0.25 -> 0.125 (floor).
 *
 * @param port_index Zero-based index of the port (0 = first, 1 = second, etc.).
 * @return Diminishing returns multiplier (0.125 to 1.0).
 */
inline float apply_diminishing_returns(float base_bonus, uint32_t port_index) {
    float multiplier;

    if (port_index == 0) {
        multiplier = DIMINISHING_FIRST;
    } else if (port_index == 1) {
        multiplier = DIMINISHING_SECOND;
    } else if (port_index == 2) {
        multiplier = DIMINISHING_THIRD;
    } else {
        // 4th+ ports get the floor multiplier
        multiplier = DIMINISHING_FLOOR;
    }

    return base_bonus * multiplier;
}

/**
 * @brief Get the raw diminishing returns multiplier for a port index.
 *
 * Similar to apply_diminishing_returns but returns just the multiplier
 * without applying it to a base bonus.
 *
 * @param port_index Zero-based index of the port.
 * @return Multiplier value (0.125 to 1.0).
 */
inline float get_diminishing_multiplier(uint32_t port_index) {
    if (port_index == 0) return DIMINISHING_FIRST;
    if (port_index == 1) return DIMINISHING_SECOND;
    if (port_index == 2) return DIMINISHING_THIRD;
    return DIMINISHING_FLOOR;
}

/**
 * @brief Calculate the global demand bonus with diminishing returns for same-type ports.
 *
 * This replaces the simple stacking behavior of calculate_global_demand_bonus
 * by applying diminishing returns when a player has multiple ports of the same type.
 *
 * Ports are processed in order of appearance (first in the vector = first port).
 * Only operational ports owned by the specified owner and matching the zone type's
 * contributing port type are counted.
 *
 * The zone_type parameter determines which port type is checked:
 * - zone_type 1 (Exchange):    sums bonuses from Aero ports (with diminishing returns)
 * - zone_type 2 (Fabrication): sums bonuses from Aqua ports (with diminishing returns)
 * - Other zone types:          returns 0.0
 *
 * Result is capped at MAX_TOTAL_DEMAND_BONUS (30.0).
 *
 * @param zone_type The zone type to calculate bonus for (1=Exchange, 2=Fabrication).
 * @param owner Owner player ID to filter ports.
 * @param ports Vector of all port data.
 * @return Demand bonus with diminishing returns (0.0 to MAX_TOTAL_DEMAND_BONUS).
 */
inline float calculate_global_demand_bonus_with_diminishing(uint8_t zone_type, uint8_t owner,
                                                              const std::vector<PortData>& ports) {
    // Determine which port type contributes to this zone type
    PortType contributing_port_type;
    if (zone_type == 1) {
        contributing_port_type = PortType::Aero;
    } else if (zone_type == 2) {
        contributing_port_type = PortType::Aqua;
    } else {
        return 0.0f;
    }

    float total_bonus = 0.0f;
    uint32_t port_index = 0;

    for (const auto& port : ports) {
        // Only count operational ports of the correct type owned by this player
        if (port.port_type != contributing_port_type) continue;
        if (port.owner != owner) continue;
        if (!port.is_operational) continue;

        float base_bonus = get_port_size_bonus(port.capacity);
        total_bonus += apply_diminishing_returns(base_bonus, port_index);

        ++port_index;
    }

    // Cap at maximum
    return std::min(total_bonus, MAX_TOTAL_DEMAND_BONUS);
}

} // namespace port
} // namespace sims3000
