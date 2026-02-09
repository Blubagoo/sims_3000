/**
 * @file GlobalServiceAggregation.h
 * @brief Capacity-based effectiveness for global services (Ticket E9-023)
 *
 * Global services (Medical, Education) don't use spatial coverage grids.
 * Instead, total capacity from all buildings of a given type is summed
 * and compared against population to determine a single effectiveness
 * value per player.
 *
 * Formula: effectiveness = clamp(total_capacity / population, 0.0, 1.0)
 * - If population == 0, effectiveness = 1.0 (fully covered)
 * - Funding modifier is applied after the capacity/population ratio
 * - Final result is clamped to [0.0, 1.0]
 *
 * @see ServiceConfigs.h for BEINGS_PER_MEDICAL_UNIT, BEINGS_PER_EDUCATION_UNIT
 * @see FundingModifier.h for funding curve
 */

#pragma once

#include <sims3000/services/CoverageCalculation.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace services {

// ServiceBuildingData is defined in CoverageCalculation.h with all needed fields
// including: type, capacity, is_active, x, y, tier, effectiveness, owner_id

// ============================================================================
// Global Service Data (output of aggregation)
// ============================================================================

/**
 * @struct GlobalServiceData
 * @brief Result of global service effectiveness calculation.
 *
 * Contains the summed capacity from all active buildings and
 * the computed effectiveness ratio for a service type.
 */
struct GlobalServiceData {
    uint32_t total_capacity = 0;    ///< Sum of capacity from all active buildings
    float effectiveness = 0.0f;     ///< Effectiveness ratio (0.0 to 1.0)
};

// ============================================================================
// Global Service Aggregation Functions
// ============================================================================

/**
 * @brief Get the beings-per-unit constant for a service type.
 *
 * Returns the BEINGS_PER_*_UNIT constant for capacity-based services.
 * Returns 0 for radius-based services (Enforcer, HazardResponse) since
 * they don't use capacity-based aggregation.
 *
 * @param type The service type to query.
 * @return Beings-per-unit value (500 for Medical, 300 for Education, 0 otherwise).
 */
uint32_t get_beings_per_unit(ServiceType type);

/**
 * @brief Calculate global service effectiveness for a service type.
 *
 * Sums capacity from all active buildings of the matching type, then
 * calculates effectiveness as: total_capacity / population.
 *
 * Special cases:
 * - population == 0: effectiveness = 1.0 (fully covered)
 * - No active buildings: effectiveness = 0.0
 *
 * The funding_percent modifier scales the final effectiveness:
 * - 100% funding = 1.0x multiplier (default)
 * - 50% funding = 0.5x multiplier
 * - 150% funding = 1.15x multiplier (capped)
 *
 * Final result is clamped to [0.0, 1.0].
 *
 * @param type           Service type to aggregate (should be Medical or Education).
 * @param buildings      Vector of service building data to aggregate.
 * @param population     Current population count (number of beings).
 * @param funding_percent Funding level as a percentage (0-255). Default = 100.
 * @return GlobalServiceData with total_capacity and effectiveness.
 */
GlobalServiceData calculate_global_service(
    ServiceType type,
    const std::vector<ServiceBuildingData>& buildings,
    uint32_t population,
    uint8_t funding_percent = 100
);

} // namespace services
} // namespace sims3000
