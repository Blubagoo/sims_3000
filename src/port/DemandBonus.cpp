/**
 * @file DemandBonus.cpp
 * @brief Demand bonus calculation implementation (Epic 8, Tickets E8-016, E8-017)
 *
 * Calculates demand bonuses from operational ports per zone type:
 * - Global (E8-016): Aero ports -> Exchange demand, Aqua ports -> Fabrication demand
 * - Local (E8-017): Radius-based bonuses near ports using Manhattan distance
 */

#include <sims3000/port/DemandBonus.h>
#include <algorithm>
#include <cstdlib>

namespace sims3000 {
namespace port {

// =============================================================================
// Port Size Bonus
// =============================================================================

float get_port_size_bonus(uint16_t capacity) {
    if (capacity == 0) {
        return 0.0f;
    }
    if (capacity < PORT_SIZE_MEDIUM_MIN) {
        // Small: capacity 1-499
        return DEMAND_BONUS_SMALL;
    }
    if (capacity <= PORT_SIZE_MEDIUM_MAX) {
        // Medium: capacity 500-1999
        return DEMAND_BONUS_MEDIUM;
    }
    // Large: capacity >= 2000
    return DEMAND_BONUS_LARGE;
}

// =============================================================================
// Global Demand Bonus
// =============================================================================

float calculate_global_demand_bonus(uint8_t zone_type, uint8_t owner,
                                     const std::vector<PortData>& ports) {
    // Determine which port type contributes to this zone type
    // Exchange (1) <- Aero ports
    // Fabrication (2) <- Aqua ports
    PortType contributing_port_type;
    if (zone_type == 1) {
        contributing_port_type = PortType::Aero;
    } else if (zone_type == 2) {
        contributing_port_type = PortType::Aqua;
    } else {
        // No port demand bonus for Habitation or other zone types
        return 0.0f;
    }

    float total_bonus = 0.0f;

    for (const auto& port : ports) {
        // Only count operational ports of the correct type owned by this player
        if (port.port_type != contributing_port_type) continue;
        if (port.owner != owner) continue;
        if (!port.is_operational) continue;

        total_bonus += get_port_size_bonus(port.capacity);
    }

    // Cap at maximum
    return std::min(total_bonus, MAX_TOTAL_DEMAND_BONUS);
}

// =============================================================================
// Local Demand Bonus (E8-017)
// =============================================================================

float calculate_local_demand_bonus(uint8_t zone_type, int32_t x, int32_t y,
                                     uint8_t owner,
                                     const std::vector<PortData>& ports) {
    // Determine which port type provides local bonus to this zone type
    // Habitation (0) <- Aero ports (radius 20, +5%)
    // Exchange (1) <- Aqua ports (radius 25, +10%)
    PortType contributing_port_type;
    int32_t radius;
    float bonus_per_port;

    if (zone_type == 0) {
        // Habitation zones get local bonus from Aero ports
        contributing_port_type = PortType::Aero;
        radius = LOCAL_BONUS_AERO_RADIUS;
        bonus_per_port = LOCAL_BONUS_AERO_HABITATION;
    } else if (zone_type == 1) {
        // Exchange zones get local bonus from Aqua ports
        contributing_port_type = PortType::Aqua;
        radius = LOCAL_BONUS_AQUA_RADIUS;
        bonus_per_port = LOCAL_BONUS_AQUA_EXCHANGE;
    } else {
        // No local bonus for Fabrication or other zone types
        return 0.0f;
    }

    float total_bonus = 0.0f;

    for (const auto& port : ports) {
        // Only count operational ports of the correct type owned by this player
        if (port.port_type != contributing_port_type) continue;
        if (port.owner != owner) continue;
        if (!port.is_operational) continue;

        // Manhattan distance check
        int32_t dist = std::abs(port.x - x) + std::abs(port.y - y);
        if (dist <= radius) {
            total_bonus += bonus_per_port;
        }
    }

    return total_bonus;
}

// =============================================================================
// Combined Demand Bonus (Global + Local, capped)
// =============================================================================

float calculate_combined_demand_bonus(uint8_t zone_type, int32_t x, int32_t y,
                                        uint8_t owner,
                                        const std::vector<PortData>& ports) {
    float global = calculate_global_demand_bonus(zone_type, owner, ports);
    float local = calculate_local_demand_bonus(zone_type, x, y, owner, ports);
    return std::min(global + local, MAX_TOTAL_DEMAND_BONUS);
}

} // namespace port
} // namespace sims3000
