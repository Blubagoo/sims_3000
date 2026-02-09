/**
 * @file FabricationDemand.h
 * @brief Fabrication (industrial) zone demand formula (Ticket E10-045)
 *
 * Pure calculation function that computes industrial demand based on
 * fabrication job coverage, labor surplus, external connectivity, and
 * congestion level. Returns a demand value in [-100, +100] with a
 * breakdown of contributing factors.
 *
 * @see E10-045
 */

#ifndef SIMS3000_DEMAND_FABRICATIONDEMAND_H
#define SIMS3000_DEMAND_FABRICATIONDEMAND_H

#include <cstdint>
#include "sims3000/demand/DemandData.h"

namespace sims3000 {
namespace demand {

/**
 * @struct FabricationInputs
 * @brief Input parameters for fabrication demand calculation.
 */
struct FabricationInputs {
    uint32_t total_beings = 0;              ///< Current population count
    uint32_t fabrication_jobs = 0;          ///< Current fabrication (industrial) job count
    uint32_t labor_force = 0;              ///< Number of beings in the labor force
    uint32_t employed_laborers = 0;         ///< Number of currently employed laborers
    bool has_external_connectivity = false;  ///< Whether city has external transport links
    float congestion_level = 0.0f;          ///< Transport congestion (0-100)
};

/**
 * @struct FabricationDemandResult
 * @brief Output of fabrication demand calculation.
 */
struct FabricationDemandResult {
    int8_t demand;          ///< Net demand value clamped to [-100, +100]
    DemandFactors factors;  ///< Breakdown of individual contributing factors
};

/**
 * @brief Calculate fabrication (industrial) zone demand.
 *
 * Computes demand based on:
 * - Population factor: fabrication job coverage ratio (under-served = positive)
 * - Employment factor: labor surplus availability
 * - Transport factor: external connectivity bonus/penalty minus congestion
 * - Contamination factor: 0 (fabrication is contamination-tolerant)
 *
 * @param inputs Fabrication demand input parameters.
 * @return FabricationDemandResult with clamped demand and factor breakdown.
 */
FabricationDemandResult calculate_fabrication_demand(const FabricationInputs& inputs);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_FABRICATIONDEMAND_H
