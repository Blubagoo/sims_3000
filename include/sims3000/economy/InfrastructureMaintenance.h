/**
 * @file InfrastructureMaintenance.h
 * @brief Pure calculation module for infrastructure upkeep costs (Ticket E11-009)
 *
 * Defines infrastructure types, per-tile maintenance rates, and functions
 * to calculate per-entity and aggregate infrastructure maintenance costs.
 *
 * Uses MaintenanceCostComponent (base_cost, cost_multiplier) for per-entity input.
 * Performance: O(1) per entity calculation.
 */

#ifndef SIMS3000_ECONOMY_INFRASTRUCTUREMAINTENANCE_H
#define SIMS3000_ECONOMY_INFRASTRUCTUREMAINTENANCE_H

#include <cstdint>
#include <vector>
#include <utility>

namespace sims3000 {
namespace economy {

// ============================================================================
// Infrastructure Type Enum
// ============================================================================

/**
 * @enum InfrastructureType
 * @brief Categories of infrastructure that incur maintenance costs.
 */
enum class InfrastructureType : uint8_t {
    Pathway      = 0,   ///< Roads, sidewalks
    EnergyConduit = 1,  ///< Power lines, energy distribution
    FluidConduit  = 2,  ///< Water/sewage pipes
    RailTrack     = 3   ///< Rail transport tracks
};

// ============================================================================
// Maintenance Rate Constants (per tile per phase)
// ============================================================================

constexpr int32_t MAINTENANCE_PATHWAY       = 5;  ///< Pathway maintenance per tile per phase
constexpr int32_t MAINTENANCE_ENERGY_CONDUIT = 2;  ///< Energy conduit maintenance per tile per phase
constexpr int32_t MAINTENANCE_FLUID_CONDUIT  = 3;  ///< Fluid conduit maintenance per tile per phase
constexpr int32_t MAINTENANCE_RAIL_TRACK     = 8;  ///< Rail track maintenance per tile per phase

// ============================================================================
// Calculation Structures
// ============================================================================

/**
 * @struct InfrastructureMaintenanceInput
 * @brief Input data for per-entity infrastructure maintenance calculation.
 *
 * Mirrors fields from MaintenanceCostComponent.
 */
struct InfrastructureMaintenanceInput {
    int32_t base_cost;       ///< From MaintenanceCostComponent
    float cost_multiplier;   ///< From MaintenanceCostComponent (age, damage)
};

/**
 * @struct InfrastructureMaintenanceResult
 * @brief Aggregated infrastructure maintenance costs by category.
 */
struct InfrastructureMaintenanceResult {
    int64_t pathway_cost;         ///< Total pathway maintenance
    int64_t energy_conduit_cost;  ///< Total energy conduit maintenance
    int64_t fluid_conduit_cost;   ///< Total fluid conduit maintenance
    int64_t rail_track_cost;      ///< Total rail track maintenance
    int64_t total;                ///< Sum of all categories
};

// ============================================================================
// Calculation Functions
// ============================================================================

/**
 * @brief Calculate maintenance cost for a single infrastructure entity.
 *
 * Computes: base_cost * cost_multiplier, rounded to nearest integer.
 * Performance: O(1).
 *
 * @param input The maintenance input parameters.
 * @return Calculated maintenance cost.
 */
int64_t calculate_infrastructure_cost(const InfrastructureMaintenanceInput& input);

/**
 * @brief Get the default maintenance rate for an infrastructure type.
 *
 * @param type The infrastructure type.
 * @return Per-tile per-phase maintenance rate, or 0 for unknown types.
 */
int32_t get_infrastructure_maintenance_rate(InfrastructureType type);

/**
 * @brief Aggregate per-category maintenance costs from a vector of (type, cost) pairs.
 *
 * Sums costs by infrastructure category and computes the total.
 *
 * @param costs Vector of (InfrastructureType, cost) pairs.
 * @return Aggregated maintenance result with per-category and total costs.
 */
InfrastructureMaintenanceResult aggregate_infrastructure_maintenance(
    const std::vector<std::pair<InfrastructureType, int64_t>>& costs);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_INFRASTRUCTUREMAINTENANCE_H
