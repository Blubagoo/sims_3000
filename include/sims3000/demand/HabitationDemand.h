/**
 * @file HabitationDemand.h
 * @brief Habitation (residential) zone demand formula (Ticket E10-043)
 *
 * Pure calculation function that computes residential demand based on
 * occupancy ratio, employment balance, service coverage, tribute rate,
 * and contamination level. Returns a demand value in [-100, +100] with
 * a breakdown of contributing factors.
 *
 * @see E10-043
 */

#ifndef SIMS3000_DEMAND_HABITATIONDEMAND_H
#define SIMS3000_DEMAND_HABITATIONDEMAND_H

#include <cstdint>
#include "sims3000/demand/DemandData.h"

namespace sims3000 {
namespace demand {

/**
 * @struct HabitationInputs
 * @brief Input parameters for habitation demand calculation.
 */
struct HabitationInputs {
    uint32_t total_beings = 0;          ///< Current population count
    uint32_t housing_capacity = 0;      ///< Total available housing units
    uint32_t labor_force = 0;           ///< Number of beings in the labor force
    uint32_t total_jobs = 0;            ///< Total available jobs across all sectors
    float service_coverage = 0.0f;      ///< Service coverage percentage (0-100)
    float tribute_rate = 7.0f;          ///< Tax/tribute rate percentage
    float contamination_level = 0.0f;   ///< Environmental contamination (0-100)
};

/**
 * @struct HabitationDemandResult
 * @brief Output of habitation demand calculation.
 */
struct HabitationDemandResult {
    int8_t demand;          ///< Net demand value clamped to [-100, +100]
    DemandFactors factors;  ///< Breakdown of individual contributing factors
};

/**
 * @brief Calculate habitation (residential) zone demand.
 *
 * Computes demand based on:
 * - Population factor: occupancy ratio pressure (+30 when >0.9, -10 when <0.5)
 * - Employment factor: job availability (+20 when jobs > labor, -15 when labor > 2x jobs)
 * - Services factor: coverage quality deviation from 50%
 * - Tribute factor: tax rate impact (lower tribute = more demand)
 * - Contamination factor: environmental penalty
 *
 * @param inputs Habitation demand input parameters.
 * @return HabitationDemandResult with clamped demand and factor breakdown.
 */
HabitationDemandResult calculate_habitation_demand(const HabitationInputs& inputs);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_HABITATIONDEMAND_H
