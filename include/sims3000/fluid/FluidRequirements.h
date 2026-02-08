/**
 * @file FluidRequirements.h
 * @brief Fluid requirement constants and lookup for structure templates (Epic 6, ticket 6-039)
 *
 * Defines fluid consumption values per tick by zone type and density level.
 * These constants drive the fluid distribution system: each structure consumes
 * fluid proportional to its type and density.
 *
 * Zone types:
 * - Habitation: lowest fluid consumers (residential)
 * - Exchange: moderate fluid consumers (commercial)
 * - Fabrication: highest fluid consumers (industrial)
 *
 * Service buildings consume a fixed amount depending on size class.
 * Infrastructure (conduits, reservoirs, extractors) produce fluid rather
 * than consuming it.
 *
 * Per CCR-007, fluid requirement values MATCH energy requirements exactly.
 *
 * @see /docs/epics/epic-6/tickets.md (ticket 6-039)
 * @see EnergyRequirements.h for the mirrored energy values
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace fluid {

// =============================================================================
// Fluid Requirements Per Tick - Zone Buildings
// =============================================================================

/// Habitation (residential) - low density: modest fluid draw
constexpr uint32_t FLUID_REQ_HABITATION_LOW = 5;

/// Habitation (residential) - high density: 4x low density
constexpr uint32_t FLUID_REQ_HABITATION_HIGH = 20;

/// Exchange (commercial) - low density: double habitation low
constexpr uint32_t FLUID_REQ_EXCHANGE_LOW = 10;

/// Exchange (commercial) - high density: 4x low density
constexpr uint32_t FLUID_REQ_EXCHANGE_HIGH = 40;

/// Fabrication (industrial) - low density: triple habitation low
constexpr uint32_t FLUID_REQ_FABRICATION_LOW = 15;

/// Fabrication (industrial) - high density: 4x low density
constexpr uint32_t FLUID_REQ_FABRICATION_HIGH = 60;

// =============================================================================
// Fluid Requirements Per Tick - Service Buildings
// =============================================================================

/// Small service building (e.g. enforcer outpost, basic clinic)
constexpr uint32_t FLUID_REQ_SERVICE_SMALL = 20;

/// Medium service building (e.g. education nexus, recreation hub)
constexpr uint32_t FLUID_REQ_SERVICE_MEDIUM = 35;

/// Large service building (e.g. medical nexus, command nexus)
constexpr uint32_t FLUID_REQ_SERVICE_LARGE = 50;

// =============================================================================
// Fluid Requirements Per Tick - Infrastructure
// =============================================================================

/// Infrastructure (conduits, reservoirs, extractors produce fluid, not consume)
constexpr uint32_t FLUID_REQ_INFRASTRUCTURE = 0;

// =============================================================================
// Lookup Helpers
// =============================================================================

/**
 * @brief Get fluid requirement for a zone type and density combination.
 *
 * Maps (zone_type, density) pairs to the corresponding fluid requirement
 * constant. Returns 0 for unknown combinations.
 *
 * @param zone_type Zone building type: 0=Habitation, 1=Exchange, 2=Fabrication
 * @param density   Density level: 0=Low, 1=High
 * @return Fluid required per tick (uint32_t). Returns 0 for invalid inputs.
 */
uint32_t get_zone_fluid_requirement(uint8_t zone_type, uint8_t density);

/**
 * @brief Get fluid requirement for a service building type.
 *
 * Maps service_type to the corresponding fluid requirement constant.
 * Returns FLUID_REQ_SERVICE_MEDIUM for unknown types as a safe default.
 *
 * @param service_type Service building size class: 0=Small, 1=Medium, 2=Large
 * @return Fluid required per tick (uint32_t).
 */
uint32_t get_service_fluid_requirement(uint8_t service_type);

} // namespace fluid
} // namespace sims3000
