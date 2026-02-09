/**
 * @file LaborPoolCalculation.h
 * @brief Labor pool calculation for population simulation (Ticket E10-019)
 *
 * Pure calculation function that computes labor force size from population
 * demographics and employment data. No side effects.
 */

#ifndef SIMS3000_POPULATION_LABORPOOLCALCULATION_H
#define SIMS3000_POPULATION_LABORPOOLCALCULATION_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"

namespace sims3000 {
namespace population {

namespace constants {
    constexpr float BASE_LABOR_PARTICIPATION = 0.65f;           ///< 65% base participation
    constexpr float HARMONY_PARTICIPATION_BONUS = 0.10f;        ///< +10% at max harmony
    constexpr float EDUCATION_PARTICIPATION_BONUS = 0.10f;      ///< +10% at max education
} // namespace constants

/**
 * @struct LaborPoolResult
 * @brief Result of labor pool calculation.
 *
 * Contains working-age population, participation rate, and labor force size.
 */
struct LaborPoolResult {
    uint32_t working_age_beings;        ///< Number of working-age beings
    float labor_participation_rate;     ///< Effective participation rate (0-1)
    uint32_t labor_force;               ///< Total labor force size
};

/**
 * @brief Calculate labor pool for a population.
 *
 * Pure calculation function with no side effects. Computes labor force based on:
 * - Working-age beings derived from adult_percent
 * - Base participation rate boosted by harmony and education indices
 * - Participation rate clamped to [0, 1]
 *
 * @param pop Population data for the city
 * @param emp Employment data for the city
 * @return LaborPoolResult with working-age count, participation rate, and labor force
 */
LaborPoolResult calculate_labor_pool(const PopulationData& pop, const EmploymentData& emp);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_LABORPOOLCALCULATION_H
