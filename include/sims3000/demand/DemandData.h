/**
 * @file DemandData.h
 * @brief Demand component data structures for the demand system (Epic 10)
 *
 * Defines DemandFactors and DemandData structures that track zone growth
 * pressure for habitation, exchange, and fabrication zones. Each zone type
 * has a raw demand value (-100 to +100), a capacity cap, and a breakdown
 * of contributing factors for UI display.
 *
 * @see E10-040
 */

#ifndef SIMS3000_DEMAND_DEMANDDATA_H
#define SIMS3000_DEMAND_DEMANDDATA_H

#include <cstdint>

namespace sims3000 {
namespace demand {

/**
 * @struct DemandFactors
 * @brief Breakdown of individual factors contributing to demand for a zone type.
 *
 * Each factor is an int8_t in the range [-100, +100]. Positive values indicate
 * upward pressure on demand; negative values indicate downward pressure.
 * Used by the UI to show players why demand is high or low.
 */
struct DemandFactors {
    int8_t population_factor = 0;      ///< Population pressure (+/- demand)
    int8_t employment_factor = 0;      ///< Employment balance factor
    int8_t services_factor = 0;        ///< Service coverage quality factor
    int8_t tribute_factor = 0;         ///< Tax/tribute rate impact
    int8_t transport_factor = 0;       ///< Transport accessibility factor
    int8_t contamination_factor = 0;   ///< Environmental contamination impact
};

/**
 * @struct DemandData
 * @brief Per-player demand state for the three zone types.
 *
 * Stores raw demand values, capacity caps, and factor breakdowns for
 * habitation (residential), exchange (commercial), and fabrication (industrial)
 * zones. Target size: ~40 bytes.
 */
struct DemandData {
    // Raw demand values (-100 to +100)
    int8_t habitation_demand = 0;      ///< Residential zone demand
    int8_t exchange_demand = 0;        ///< Commercial zone demand
    int8_t fabrication_demand = 0;     ///< Industrial zone demand

    // Demand caps (maximum buildings that can grow)
    uint32_t habitation_cap = 0;       ///< Max residential growth capacity
    uint32_t exchange_cap = 0;         ///< Max commercial growth capacity
    uint32_t fabrication_cap = 0;      ///< Max industrial growth capacity

    // Factor breakdowns for UI display
    DemandFactors habitation_factors;  ///< Residential demand factor breakdown
    DemandFactors exchange_factors;    ///< Commercial demand factor breakdown
    DemandFactors fabrication_factors; ///< Industrial demand factor breakdown
};

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDDATA_H
