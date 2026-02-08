/**
 * @file EnergyPriorities.h
 * @brief Energy priority assignments per structure type (Epic 5, ticket 5-038)
 *
 * Defines priority levels for energy rationing. When total energy demand
 * exceeds supply, the distribution system allocates power to structures
 * in priority order: lower priority numbers receive power first.
 *
 * Priority levels:
 * 1 (CRITICAL)  - Essential services: medical nexus, command nexus
 * 2 (IMPORTANT) - Safety services: enforcer post, hazard response
 * 3 (NORMAL)    - Economic structures: exchange, fabrication, education, recreation
 * 4 (LOW)       - Habitation structures (last to lose power)
 *
 * @see /docs/epics/epic-5/tickets.md (ticket 5-038)
 * @see EnergyRequirements.h for energy consumption values
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace energy {

// =============================================================================
// Priority Level Constants
// =============================================================================

/// Priority 1: Critical infrastructure (medical nexus, command nexus)
constexpr uint8_t ENERGY_PRIORITY_CRITICAL = 1;

/// Priority 2: Important services (enforcer post, hazard response)
constexpr uint8_t ENERGY_PRIORITY_IMPORTANT = 2;

/// Priority 3: Normal operations (exchange, fabrication, education, recreation)
constexpr uint8_t ENERGY_PRIORITY_NORMAL = 3;

/// Priority 4: Low priority (habitation structures)
constexpr uint8_t ENERGY_PRIORITY_LOW = 4;

/// Default priority for unknown or unspecified structure types
constexpr uint8_t ENERGY_PRIORITY_DEFAULT = ENERGY_PRIORITY_NORMAL;

// =============================================================================
// Lookup Helper
// =============================================================================

/**
 * @brief Get energy priority for a zone building type.
 *
 * Maps zone types to their default rationing priority:
 * - Habitation  (0) -> ENERGY_PRIORITY_LOW (4)
 * - Exchange    (1) -> ENERGY_PRIORITY_NORMAL (3)
 * - Fabrication (2) -> ENERGY_PRIORITY_NORMAL (3)
 * - Unknown          -> ENERGY_PRIORITY_DEFAULT (3)
 *
 * Service buildings have varying priorities not captured here;
 * their priority is determined by the specific service type
 * (e.g. medical = CRITICAL, enforcer = IMPORTANT).
 *
 * @param zone_type Zone building type: 0=Habitation, 1=Exchange, 2=Fabrication
 * @return Energy priority level (1=highest, 4=lowest).
 */
uint8_t get_energy_priority_for_zone(uint8_t zone_type);

} // namespace energy
} // namespace sims3000
