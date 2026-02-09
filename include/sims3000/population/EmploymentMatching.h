/**
 * @file EmploymentMatching.h
 * @brief Employment matching algorithm (Ticket E10-021)
 *
 * Matches available labor force to jobs across exchange (commercial)
 * and fabrication (industrial) sectors. Produces employment counts,
 * unemployment count, and unemployment rate.
 */

#ifndef SIMS3000_POPULATION_EMPLOYMENTMATCHING_H
#define SIMS3000_POPULATION_EMPLOYMENTMATCHING_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @struct EmploymentMatchingResult
 * @brief Result of employment matching algorithm.
 *
 * Contains per-sector employment counts, total unemployed,
 * and unemployment rate as a percentage.
 */
struct EmploymentMatchingResult {
    uint32_t employed_laborers;       ///< Total employed beings
    uint32_t unemployed;              ///< Unemployed beings seeking work
    uint32_t exchange_employed;       ///< Employed in commercial sector
    uint32_t fabrication_employed;    ///< Employed in industrial sector
    uint8_t unemployment_rate;        ///< Percentage unemployed (0-100)
};

/**
 * @brief Match labor force to available jobs.
 *
 * Proportionally distributes employment across exchange and fabrication
 * sectors based on available jobs in each sector:
 * - total_jobs = exchange_jobs + fabrication_jobs
 * - max_employment = min(labor_force, total_jobs)
 * - exchange_employed = max_employment * (exchange_jobs / total_jobs)
 * - fabrication_employed = max_employment - exchange_employed
 * - unemployment_rate = (labor_force > 0) ? (unemployed * 100 / labor_force) : 0
 *
 * Edge case: if total_jobs == 0, all laborers are unemployed.
 *
 * @param labor_force Total beings in the labor force
 * @param exchange_jobs Available commercial sector jobs
 * @param fabrication_jobs Available industrial sector jobs
 * @return EmploymentMatchingResult with employment breakdown
 */
EmploymentMatchingResult match_employment(uint32_t labor_force,
                                           uint32_t exchange_jobs,
                                           uint32_t fabrication_jobs);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_EMPLOYMENTMATCHING_H
