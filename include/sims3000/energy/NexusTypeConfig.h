/**
 * @file NexusTypeConfig.h
 * @brief Nexus type definitions and base stats for Epic 5 (Ticket 5-023)
 *
 * Defines the static configuration data for each energy nexus type:
 * - Base output, build/maintenance costs
 * - Contamination output
 * - Coverage radius
 * - Aging floor (minimum efficiency)
 * - Terrain requirements
 * - Variable output flag (for Wind/Solar)
 *
 * Values defined per CCR-004 (Energy Nexus Balance Document).
 *
 * @see EnergyEnums.h for NexusType, TerrainRequirement enums
 */

#pragma once

#include <sims3000/energy/EnergyEnums.h>
#include <cstdint>

namespace sims3000 {
namespace energy {

/**
 * @struct NexusTypeConfig
 * @brief Static configuration data for an energy nexus type.
 *
 * Each nexus type has fixed base stats that determine its behavior.
 * Runtime values (like current efficiency) are tracked separately
 * in ECS components.
 */
struct NexusTypeConfig {
    NexusType type;                 ///< Nexus type identifier
    const char* name;               ///< Human-readable nexus name
    uint32_t base_output;           ///< Energy units produced per tick
    uint32_t build_cost;            ///< Credits to construct
    uint32_t maintenance_cost;      ///< Credits per tick to maintain
    uint32_t contamination;         ///< Contamination units per tick when online
    uint8_t coverage_radius;        ///< Coverage radius in tiles
    float aging_floor;              ///< Minimum efficiency (asymptotic aging limit)
    TerrainRequirement terrain_req; ///< Terrain requirement for placement
    bool is_variable_output;        ///< True for weather-dependent types (Wind/Solar)
};

/// Array of all MVP nexus type configurations (indexed by NexusType ordinal)
extern const NexusTypeConfig NEXUS_CONFIGS[NEXUS_TYPE_MVP_COUNT];

/**
 * @brief Look up nexus configuration by type.
 *
 * Returns the static configuration for the given NexusType.
 * Only MVP types (Carbon through Solar) are supported.
 * Passing a non-MVP type returns the Carbon config as a fallback.
 *
 * @param type The NexusType to look up.
 * @return Reference to the NexusTypeConfig for the given type.
 */
const NexusTypeConfig& get_nexus_config(NexusType type);

} // namespace energy
} // namespace sims3000
