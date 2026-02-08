/**
 * @file PortOperational.cpp
 * @brief Port operational status check implementation for Epic 8 (Ticket E8-011)
 *
 * Implements operational status checks combining:
 * - Zone validation (size, terrain requirements)
 * - Infrastructure requirements (runway or dock)
 * - Pathway connectivity
 * - Non-zero capacity check
 */

#include <sims3000/port/PortOperational.h>
#include <sims3000/port/PortZoneValidation.h>
#include <sims3000/port/PortCapacity.h>
#include <sims3000/terrain/TerrainEvents.h>

namespace sims3000 {
namespace port {

OperationalCheckResult check_port_operational(
    const PortComponent& port,
    const PortZoneComponent& zone,
    const terrain::ITerrainQueryable& terrain,
    const building::ITransportProvider& transport) {

    OperationalCheckResult result;

    // Build a GridRect from zone's runway_area for zone validation.
    // The zone's runway_area stores the overall zone bounding rect.
    terrain::GridRect zone_rect = zone.runway_area;

    // If zone_rect is empty, construct one from zone_tiles (approximate as square).
    // This handles the case where runway_area is used for the zone bounding rect.
    if (zone_rect.isEmpty()) {
        // Cannot validate without a zone rect - zone is invalid
        result.zone_valid = false;
        result.infrastructure_met = false;
        result.pathway_connected = false;
        result.has_capacity = false;
        return result;
    }

    // 1. Zone validation (size + terrain requirements)
    if (zone.port_type == PortType::Aero) {
        result.zone_valid = validate_aero_port_zone(zone_rect, terrain, transport);
    } else {
        result.zone_valid = validate_aqua_port_zone(zone_rect, terrain, transport);
    }

    // 2. Infrastructure requirements
    if (zone.port_type == PortType::Aero) {
        result.infrastructure_met = zone.has_runway;
    } else {
        result.infrastructure_met = zone.has_dock;
    }

    // 3. Pathway connectivity - check if any perimeter tile has road access
    // We reuse the transport provider check. If zone validation passed,
    // pathway was already confirmed. But we check independently for the
    // result struct to provide granular feedback.
    {
        bool connected = false;
        // Check perimeter tiles for road access within PORT_PATHWAY_MAX_DISTANCE
        // Top and bottom edges
        for (int16_t x = zone_rect.x; x < zone_rect.right() && !connected; ++x) {
            if (transport.is_road_accessible_at(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(zone_rect.y),
                    PORT_PATHWAY_MAX_DISTANCE)) {
                connected = true;
            }
            if (!connected && zone_rect.height > 1) {
                if (transport.is_road_accessible_at(
                        static_cast<uint32_t>(x),
                        static_cast<uint32_t>(zone_rect.bottom() - 1),
                        PORT_PATHWAY_MAX_DISTANCE)) {
                    connected = true;
                }
            }
        }
        // Left and right edges (excluding corners)
        for (int16_t y = static_cast<int16_t>(zone_rect.y + 1);
             y < static_cast<int16_t>(zone_rect.bottom() - 1) && !connected; ++y) {
            if (transport.is_road_accessible_at(
                    static_cast<uint32_t>(zone_rect.x),
                    static_cast<uint32_t>(y),
                    PORT_PATHWAY_MAX_DISTANCE)) {
                connected = true;
            }
            if (!connected && zone_rect.width > 1) {
                if (transport.is_road_accessible_at(
                        static_cast<uint32_t>(zone_rect.right() - 1),
                        static_cast<uint32_t>(y),
                        PORT_PATHWAY_MAX_DISTANCE)) {
                    connected = true;
                }
            }
        }
        result.pathway_connected = connected;
    }

    // 4. Non-zero capacity check
    result.has_capacity = (port.capacity > 0);

    return result;
}

bool update_port_operational_status(
    PortComponent& port,
    const PortZoneComponent& zone,
    const terrain::ITerrainQueryable& terrain,
    const building::ITransportProvider& transport,
    uint32_t port_entity_id,
    uint8_t owner_id,
    std::vector<PortOperationalEvent>& out_events) {

    OperationalCheckResult check = check_port_operational(port, zone, terrain, transport);
    bool new_operational = check.is_operational();
    bool old_operational = port.is_operational;

    // Update the flag
    port.is_operational = new_operational;

    // Emit event on state change
    if (new_operational != old_operational) {
        out_events.emplace_back(port_entity_id, new_operational, owner_id);
        return true;
    }

    return false;
}

} // namespace port
} // namespace sims3000
