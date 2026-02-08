/**
 * @file EnergyRequirements.h
 * @brief Energy requirement constants and lookup for structure templates (Epic 5, ticket 5-037)
 *
 * Defines energy consumption values per tick by zone type and density level.
 * These constants drive the energy distribution system: each structure consumes
 * energy proportional to its type and density.
 *
 * Zone types:
 * - Habitation: lowest energy consumers (residential)
 * - Exchange: moderate energy consumers (commercial)
 * - Fabrication: highest energy consumers (industrial)
 *
 * Service buildings consume a fixed amount depending on size class.
 * Infrastructure (conduits, nexuses) produce energy rather than consuming it.
 *
 * @see /docs/epics/epic-5/tickets.md (ticket 5-037)
 * @see EnergyPriorities.h for rationing priority assignments
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace energy {

// =============================================================================
// Energy Requirements Per Tick - Zone Buildings
// =============================================================================

/// Habitation (residential) - low density: modest energy draw
constexpr uint32_t ENERGY_REQ_HABITATION_LOW = 5;

/// Habitation (residential) - high density: 4x low density
constexpr uint32_t ENERGY_REQ_HABITATION_HIGH = 20;

/// Exchange (commercial) - low density: double habitation low
constexpr uint32_t ENERGY_REQ_EXCHANGE_LOW = 10;

/// Exchange (commercial) - high density: 4x low density
constexpr uint32_t ENERGY_REQ_EXCHANGE_HIGH = 40;

/// Fabrication (industrial) - low density: triple habitation low
constexpr uint32_t ENERGY_REQ_FABRICATION_LOW = 15;

/// Fabrication (industrial) - high density: 4x low density
constexpr uint32_t ENERGY_REQ_FABRICATION_HIGH = 60;

// =============================================================================
// Energy Requirements Per Tick - Service Buildings
// =============================================================================

/// Small service building (e.g. enforcer outpost, basic clinic)
constexpr uint32_t ENERGY_REQ_SERVICE_SMALL = 20;

/// Medium service building (e.g. education nexus, recreation hub)
constexpr uint32_t ENERGY_REQ_SERVICE_MEDIUM = 35;

/// Large service building (e.g. medical nexus, command nexus)
constexpr uint32_t ENERGY_REQ_SERVICE_LARGE = 50;

// =============================================================================
// Energy Requirements Per Tick - Infrastructure
// =============================================================================

/// Infrastructure (conduits and nexuses produce energy, not consume)
constexpr uint32_t ENERGY_REQ_INFRASTRUCTURE = 0;

// =============================================================================
// Lookup Helper
// =============================================================================

/**
 * @brief Get energy requirement for a zone type and density combination.
 *
 * Maps (zone_type, density) pairs to the corresponding energy requirement
 * constant. Returns 0 for unknown combinations.
 *
 * @param zone_type Zone building type: 0=Habitation, 1=Exchange, 2=Fabrication
 * @param density   Density level: 0=Low, 1=High
 * @return Energy required per tick (uint32_t). Returns 0 for invalid inputs.
 */
uint32_t get_energy_requirement(uint8_t zone_type, uint8_t density);

} // namespace energy
} // namespace sims3000
