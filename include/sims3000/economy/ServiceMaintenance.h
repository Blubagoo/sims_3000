/**
 * @file ServiceMaintenance.h
 * @brief Pure calculation module for service building upkeep (Ticket E11-010)
 *
 * Defines base costs for each service type and functions to calculate
 * funding-scaled maintenance costs. Costs scale linearly with funding level:
 * actual_cost = base_cost * (funding_level / 100.0f).
 *
 * Uses ServiceType from services/ServiceTypes.h for type identification.
 * Uses TreasuryState funding fields for per-service funding levels.
 */

#ifndef SIMS3000_ECONOMY_SERVICEMAINTENANCE_H
#define SIMS3000_ECONOMY_SERVICEMAINTENANCE_H

#include <cstdint>
#include <vector>
#include <utility>

namespace sims3000 {
namespace economy {

// ============================================================================
// Service Building Cost Constants (per phase at 100% funding)
// ============================================================================

constexpr int32_t SERVICE_COST_ENFORCER        = 100;  ///< Enforcer base cost per phase
constexpr int32_t SERVICE_COST_HAZARD_RESPONSE = 120;  ///< Hazard response base cost per phase
constexpr int32_t SERVICE_COST_MEDICAL         = 300;  ///< Medical base cost per phase
constexpr int32_t SERVICE_COST_EDUCATION       = 200;  ///< Education base cost per phase

// ============================================================================
// Calculation Structures
// ============================================================================

/**
 * @struct ServiceMaintenanceInput
 * @brief Input data for per-building service maintenance calculation.
 */
struct ServiceMaintenanceInput {
    uint8_t service_type;   ///< ServiceType enum value (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education)
    int32_t base_cost;      ///< From get_service_base_cost or MaintenanceCostComponent
    uint8_t funding_level;  ///< 0-150%, from TreasuryState
};

/**
 * @struct ServiceMaintenanceResult
 * @brief Result of a single service maintenance calculation.
 */
struct ServiceMaintenanceResult {
    int64_t actual_cost;    ///< cost * (funding_level / 100.0)
    float funding_factor;   ///< funding_level / 100.0
};

/**
 * @struct ServiceMaintenanceSummary
 * @brief Aggregated service maintenance costs by service type.
 */
struct ServiceMaintenanceSummary {
    int64_t enforcer_cost;         ///< Total enforcer maintenance
    int64_t hazard_response_cost;  ///< Total hazard response maintenance
    int64_t medical_cost;          ///< Total medical maintenance
    int64_t education_cost;        ///< Total education maintenance
    int64_t total;                 ///< Sum of all categories
};

// ============================================================================
// Calculation Functions
// ============================================================================

/**
 * @brief Get the base cost for a service type.
 *
 * @param service_type ServiceType enum value (0-3).
 * @return Base maintenance cost per phase, or 0 for unknown types.
 */
int32_t get_service_base_cost(uint8_t service_type);

/**
 * @brief Calculate funding-scaled maintenance cost for a single service building.
 *
 * Computes: actual_cost = base_cost * (funding_level / 100.0f)
 * The funding_factor is stored for reference.
 *
 * @param input The service maintenance input parameters.
 * @return ServiceMaintenanceResult with actual_cost and funding_factor.
 */
ServiceMaintenanceResult calculate_service_maintenance(const ServiceMaintenanceInput& input);

/**
 * @brief Aggregate per-building service maintenance costs by service type.
 *
 * Sums costs by service category and computes the total.
 *
 * @param costs Vector of (service_type, cost) pairs.
 * @return Aggregated summary with per-category and total costs.
 */
ServiceMaintenanceSummary aggregate_service_maintenance(
    const std::vector<std::pair<uint8_t, int64_t>>& costs);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_SERVICEMAINTENANCE_H
