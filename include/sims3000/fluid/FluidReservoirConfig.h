/**
 * @file FluidReservoirConfig.h
 * @brief Fluid reservoir type definition and base stats for Epic 6 (Ticket 6-024)
 *
 * Defines the static configuration data for a fluid reservoir:
 * - Capacity, fill/drain rates
 * - Build/maintenance costs
 * - Coverage radius
 * - Energy requirement flag (passive storage)
 *
 * Asymmetric rates per CCR-005: drain_rate (100) > fill_rate (50).
 * This ensures reservoirs empty faster than they fill, preventing
 * over-reliance on storage as a production substitute.
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

/// Default storage capacity (fluid units)
constexpr uint32_t RESERVOIR_DEFAULT_CAPACITY = 1000;

/// Default fill rate (fluid units per tick)
/// Deliberately slower than drain rate (CCR-005: asymmetric rates)
constexpr uint16_t RESERVOIR_DEFAULT_FILL_RATE = 50;

/// Default drain rate (fluid units per tick)
/// Deliberately faster than fill rate (CCR-005: asymmetric rates)
/// drain (100) > fill (50) to prevent storage from substituting production
constexpr uint16_t RESERVOIR_DEFAULT_DRAIN_RATE = 100;

/// Default build cost (credits)
constexpr uint32_t RESERVOIR_DEFAULT_BUILD_COST = 2000;

/// Default maintenance cost per cycle (credits)
constexpr uint32_t RESERVOIR_DEFAULT_MAINTENANCE_COST = 20;

/// Coverage radius (tiles)
constexpr uint8_t RESERVOIR_DEFAULT_COVERAGE_RADIUS = 6;

/// Whether the reservoir requires energy (passive storage does not)
constexpr bool RESERVOIR_DEFAULT_REQUIRES_ENERGY = false;

/**
 * @struct FluidReservoirConfig
 * @brief Static configuration data for a fluid reservoir.
 *
 * Each reservoir has fixed base stats that determine its behavior.
 * Runtime values (like current stored amount) are tracked separately
 * in ECS components.
 *
 * Note: Asymmetric fill/drain rates per CCR-005.
 * drain_rate (100) > fill_rate (50) -- reservoirs empty faster than
 * they fill, preventing over-reliance on storage as a substitute
 * for production capacity.
 */
struct FluidReservoirConfig {
    uint32_t capacity;           ///< Maximum fluid units stored
    uint16_t fill_rate;          ///< Fluid units absorbed per tick (slower, CCR-005)
    uint16_t drain_rate;         ///< Fluid units distributed per tick (faster, CCR-005)
    uint32_t build_cost;         ///< Credits to construct
    uint32_t maintenance_cost;   ///< Credits per maintenance cycle
    uint8_t coverage_radius;     ///< Coverage radius in tiles
    bool requires_energy;        ///< Whether this facility needs energy (false = passive)
};

/**
 * @brief Get the default fluid reservoir configuration.
 *
 * Returns a FluidReservoirConfig populated with all default values
 * from the named constants above.
 *
 * @return FluidReservoirConfig with default values.
 */
FluidReservoirConfig get_default_reservoir_config();

} // namespace fluid
} // namespace sims3000
