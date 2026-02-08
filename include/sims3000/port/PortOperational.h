/**
 * @file PortOperational.h
 * @brief Port operational status check for Epic 8 (Ticket E8-011)
 *
 * Provides operational status checks for port facilities:
 * - Zone validation (size, terrain requirements)
 * - Infrastructure requirements (runway or dock)
 * - Pathway connectivity (within 3 tiles)
 * - Non-zero capacity
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <sims3000/port/PortComponent.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortEvents.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/**
 * @struct OperationalCheckResult
 * @brief Result of an operational status check with detail flags.
 *
 * Provides individual check results for debugging and UI display.
 */
struct OperationalCheckResult {
    bool zone_valid            = false;  ///< Zone validation passed
    bool infrastructure_met    = false;  ///< Infrastructure requirements met (runway or dock)
    bool pathway_connected     = false;  ///< Connected via pathway (within 3 tiles)
    bool has_capacity          = false;  ///< Has non-zero capacity

    /**
     * @brief Check if all conditions are met for operational status.
     * @return true if all flags are true.
     */
    bool is_operational() const {
        return zone_valid && infrastructure_met && pathway_connected && has_capacity;
    }
};

/**
 * @brief Check if a port meets all operational requirements.
 *
 * A port is operational when:
 * 1. Zone validation passes (size, terrain requirements)
 * 2. Infrastructure requirements met (runway for aero, dock for aqua)
 * 3. Pathway connected (within 3 tiles of zone perimeter)
 * 4. Has non-zero capacity
 *
 * @param port The port component with current state.
 * @param zone The port zone component with zone metadata.
 * @param terrain Terrain query interface for zone validation.
 * @param transport Transport provider for pathway accessibility.
 * @return OperationalCheckResult with individual check flags.
 */
OperationalCheckResult check_port_operational(
    const PortComponent& port,
    const PortZoneComponent& zone,
    const terrain::ITerrainQueryable& terrain,
    const building::ITransportProvider& transport);

/**
 * @brief Update a port's operational status and generate events on change.
 *
 * Calls check_port_operational and updates the PortComponent::is_operational
 * flag. If the operational status changed, appends a PortOperationalEvent
 * to the output events vector.
 *
 * @param port Port component to update (modified in place).
 * @param zone Port zone component with zone metadata.
 * @param terrain Terrain query interface.
 * @param transport Transport provider.
 * @param port_entity_id Entity ID of the port facility.
 * @param owner_id Owner player ID.
 * @param out_events Output vector for generated events.
 * @return true if the operational status changed.
 */
bool update_port_operational_status(
    PortComponent& port,
    const PortZoneComponent& zone,
    const terrain::ITerrainQueryable& terrain,
    const building::ITransportProvider& transport,
    uint32_t port_entity_id,
    uint8_t owner_id,
    std::vector<PortOperationalEvent>& out_events);

} // namespace port
} // namespace sims3000
