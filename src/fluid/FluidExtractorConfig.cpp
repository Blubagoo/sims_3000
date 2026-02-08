/**
 * @file FluidExtractorConfig.cpp
 * @brief Fluid extractor configuration data (Ticket 6-023)
 *
 * Provides the default configuration for fluid extractors.
 * All values use named constants from FluidExtractorConfig.h --
 * no magic numbers.
 *
 * Values per CCR-008 (Extractor energy priority).
 */

#include <sims3000/fluid/FluidExtractorConfig.h>

namespace sims3000 {
namespace fluid {

// =============================================================================
// Default Configuration
// =============================================================================

FluidExtractorConfig get_default_extractor_config() {
    FluidExtractorConfig config{};
    config.base_output             = EXTRACTOR_DEFAULT_BASE_OUTPUT;
    config.build_cost              = EXTRACTOR_DEFAULT_BUILD_COST;
    config.maintenance_cost        = EXTRACTOR_DEFAULT_MAINTENANCE_COST;
    config.energy_required         = EXTRACTOR_DEFAULT_ENERGY_REQUIRED;
    config.energy_priority         = EXTRACTOR_DEFAULT_ENERGY_PRIORITY;
    config.max_placement_distance  = EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE;
    config.max_operational_distance = EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE;
    config.coverage_radius         = EXTRACTOR_DEFAULT_COVERAGE_RADIUS;
    return config;
}

} // namespace fluid
} // namespace sims3000
