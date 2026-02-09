/**
 * @file DemandCap.h
 * @brief Demand capacity cap calculation (Ticket E10-046)
 *
 * Pure calculation function that computes maximum growth capacity for
 * each zone type based on raw capacity and infrastructure factors
 * (energy, fluid, transport). Caps limit how many buildings can grow
 * even when demand is positive.
 *
 * @see E10-046
 */

#ifndef SIMS3000_DEMAND_DEMANDCAP_H
#define SIMS3000_DEMAND_DEMANDCAP_H

#include <cstdint>

namespace sims3000 {
namespace demand {

/**
 * @struct DemandCapInputs
 * @brief Input parameters for demand cap calculation.
 */
struct DemandCapInputs {
    uint32_t housing_capacity = 0;      ///< Total housing unit capacity
    uint32_t exchange_jobs = 0;         ///< Total exchange (commercial) job slots
    uint32_t fabrication_jobs = 0;      ///< Total fabrication (industrial) job slots
    float energy_factor = 1.0f;         ///< Powered ratio (0-1)
    float fluid_factor = 1.0f;          ///< Watered ratio (0-1)
    float transport_factor = 1.0f;      ///< Transport quality (0-1, i.e. 1 - congestion)
};

/**
 * @struct DemandCapResult
 * @brief Output of demand cap calculation.
 */
struct DemandCapResult {
    uint32_t habitation_cap;    ///< Maximum residential growth capacity
    uint32_t exchange_cap;      ///< Maximum commercial growth capacity
    uint32_t fabrication_cap;   ///< Maximum industrial growth capacity
};

/**
 * @brief Calculate demand caps for all three zone types.
 *
 * Caps are computed by multiplying raw capacity by infrastructure factors:
 * - habitation_cap = housing_capacity * energy_factor * fluid_factor
 * - exchange_cap = exchange_jobs * transport_factor
 * - fabrication_cap = fabrication_jobs * transport_factor
 *
 * @param inputs Demand cap input parameters.
 * @return DemandCapResult with computed caps for each zone type.
 */
DemandCapResult calculate_demand_caps(const DemandCapInputs& inputs);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDCAP_H
