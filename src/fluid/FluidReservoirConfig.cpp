/**
 * @file FluidReservoirConfig.cpp
 * @brief Fluid reservoir configuration data (Ticket 6-024)
 *
 * Provides the default configuration for fluid reservoirs.
 * All values use named constants from FluidReservoirConfig.h --
 * no magic numbers.
 *
 * Asymmetric rates per CCR-005: drain_rate (100) > fill_rate (50).
 */

#include <sims3000/fluid/FluidReservoirConfig.h>

namespace sims3000 {
namespace fluid {

// =============================================================================
// Default Configuration
// =============================================================================

FluidReservoirConfig get_default_reservoir_config() {
    FluidReservoirConfig config{};
    config.capacity         = RESERVOIR_DEFAULT_CAPACITY;
    config.fill_rate        = RESERVOIR_DEFAULT_FILL_RATE;
    config.drain_rate       = RESERVOIR_DEFAULT_DRAIN_RATE;
    config.build_cost       = RESERVOIR_DEFAULT_BUILD_COST;
    config.maintenance_cost = RESERVOIR_DEFAULT_MAINTENANCE_COST;
    config.coverage_radius  = RESERVOIR_DEFAULT_COVERAGE_RADIUS;
    config.requires_energy  = RESERVOIR_DEFAULT_REQUIRES_ENERGY;
    return config;
}

} // namespace fluid
} // namespace sims3000
