/**
 * @file PortCapacity.cpp
 * @brief Port capacity calculation implementation for Epic 8 (Ticket E8-010)
 *
 * Implements capacity formulas for aero and aqua ports:
 * - Aero: zone_tiles * 10 * runway_bonus * access_bonus, cap 2500
 * - Aqua: zone_tiles * 15 * dock_bonus * water_access * rail_bonus, cap 5000
 */

#include <sims3000/port/PortCapacity.h>
#include <algorithm>

namespace sims3000 {
namespace port {

uint16_t calculate_aero_capacity(const PortZoneComponent& zone,
                                  bool pathway_connected) {
    // Base capacity from zone size
    float base_capacity = static_cast<float>(zone.zone_tiles) *
                          static_cast<float>(AERO_BASE_CAPACITY_PER_TILE);

    // Runway bonus: 1.5x with runway, 0.5x without
    float runway_bonus = zone.has_runway ? AERO_RUNWAY_BONUS : AERO_NO_RUNWAY_BONUS;

    // Access bonus: 1.0 if connected, 0.0 if not (no capacity without access)
    float access_bonus = pathway_connected ? AERO_ACCESS_CONNECTED : AERO_ACCESS_DISCONNECTED;

    // Calculate final capacity
    float raw_capacity = base_capacity * runway_bonus * access_bonus;

    // Clamp to [0, AERO_PORT_MAX_CAPACITY]
    uint16_t capped = static_cast<uint16_t>(
        std::min(raw_capacity, static_cast<float>(AERO_PORT_MAX_CAPACITY)));

    return capped;
}

uint16_t calculate_aqua_capacity(const PortZoneComponent& zone,
                                  uint32_t adjacent_water,
                                  bool has_rail_connection) {
    // Base capacity from zone size
    float base_capacity = static_cast<float>(zone.zone_tiles) *
                          static_cast<float>(AQUA_BASE_CAPACITY_PER_TILE);

    // Dock bonus: 1.0 + (dock_count * 0.2)
    float dock_bonus = AQUA_DOCK_BONUS_BASE +
                       (static_cast<float>(zone.dock_count) * AQUA_DOCK_BONUS_PER_DOCK);

    // Water access: 1.0 if >= 4 adjacent water tiles, 0.5 otherwise
    float water_access = (adjacent_water >= AQUA_MIN_ADJACENT_WATER)
                         ? AQUA_WATER_ACCESS_FULL
                         : AQUA_WATER_ACCESS_PARTIAL;

    // Rail bonus: 1.5x with rail, 1.0x without
    float rail_bonus = has_rail_connection ? AQUA_RAIL_BONUS_CONNECTED
                                           : AQUA_RAIL_BONUS_DISCONNECTED;

    // Calculate final capacity
    float raw_capacity = base_capacity * dock_bonus * water_access * rail_bonus;

    // Clamp to [0, AQUA_PORT_MAX_CAPACITY]
    uint16_t capped = static_cast<uint16_t>(
        std::min(raw_capacity, static_cast<float>(AQUA_PORT_MAX_CAPACITY)));

    return capped;
}

uint16_t calculate_port_capacity(const PortZoneComponent& zone,
                                  bool pathway_connected,
                                  uint32_t adjacent_water,
                                  bool has_rail_connection) {
    switch (zone.port_type) {
        case PortType::Aero:
            return calculate_aero_capacity(zone, pathway_connected);
        case PortType::Aqua:
            return calculate_aqua_capacity(zone, adjacent_water, has_rail_connection);
        default:
            return 0;
    }
}

uint16_t get_max_capacity(PortType type) {
    switch (type) {
        case PortType::Aero: return AERO_PORT_MAX_CAPACITY;
        case PortType::Aqua: return AQUA_PORT_MAX_CAPACITY;
        default: return 0;
    }
}

} // namespace port
} // namespace sims3000
