/**
 * @file FluidExtractorConfig.h
 * @brief Fluid extractor type definition and base stats for Epic 6 (Ticket 6-023)
 *
 * Defines the static configuration data for a fluid extractor:
 * - Base output, build/maintenance costs
 * - Energy requirements and priority (CCR-008)
 * - Placement/operational distance constraints
 * - Coverage radius
 *
 * Values defined per CCR-008 (Extractor energy priority).
 *
 * @see FluidEnums.h for FluidProducerType enum
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace fluid {

// =============================================================================
// Named Constants (no magic numbers)
// =============================================================================

/// Default fluid output per tick (fluid units)
constexpr uint32_t EXTRACTOR_DEFAULT_BASE_OUTPUT = 100;

/// Default build cost (credits)
constexpr uint32_t EXTRACTOR_DEFAULT_BUILD_COST = 3000;

/// Default maintenance cost per cycle (credits)
constexpr uint32_t EXTRACTOR_DEFAULT_MAINTENANCE_COST = 30;

/// Default energy consumption (energy units for EnergyComponent)
constexpr uint32_t EXTRACTOR_DEFAULT_ENERGY_REQUIRED = 20;

/// Default energy priority: ENERGY_PRIORITY_IMPORTANT (2) per CCR-008
constexpr uint8_t EXTRACTOR_DEFAULT_ENERGY_PRIORITY = 2;

/// Maximum placement distance from water source (tiles)
constexpr uint8_t EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE = 8;

/// Maximum operational distance for full efficiency (tiles)
/// Full efficiency at 0-2 tiles; degrades beyond that
constexpr uint8_t EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE = 5;

/// Coverage radius (tiles)
constexpr uint8_t EXTRACTOR_DEFAULT_COVERAGE_RADIUS = 8;

/**
 * @struct FluidExtractorConfig
 * @brief Static configuration data for a fluid extractor.
 *
 * Each extractor has fixed base stats that determine its behavior.
 * Runtime values (like current output, efficiency) are tracked
 * separately in ECS components.
 */
struct FluidExtractorConfig {
    uint32_t base_output;              ///< Fluid units produced per tick
    uint32_t build_cost;               ///< Credits to construct
    uint32_t maintenance_cost;         ///< Credits per maintenance cycle
    uint32_t energy_required;          ///< Energy units consumed (for EnergyComponent)
    uint8_t energy_priority;           ///< Energy priority level (2 = ENERGY_PRIORITY_IMPORTANT per CCR-008)
    uint8_t max_placement_distance;    ///< Max tiles from water for valid placement
    uint8_t max_operational_distance;  ///< Max tiles from water for any efficiency (full at 0-2)
    uint8_t coverage_radius;           ///< Coverage radius in tiles
};

/**
 * @brief Get the default fluid extractor configuration.
 *
 * Returns a FluidExtractorConfig populated with all default values
 * from the named constants above.
 *
 * @return FluidExtractorConfig with default values.
 */
FluidExtractorConfig get_default_extractor_config();

} // namespace fluid
} // namespace sims3000
