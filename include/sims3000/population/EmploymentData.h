/**
 * @file EmploymentData.h
 * @brief Per-player employment statistics (Ticket E10-011)
 *
 * Tracks employment metrics across a player's entire city:
 * labor force, employed/unemployed counts by sector, and
 * commute satisfaction.
 */

#ifndef SIMS3000_POPULATION_EMPLOYMENTDATA_H
#define SIMS3000_POPULATION_EMPLOYMENTDATA_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @struct EmploymentData
 * @brief Per-player aggregate employment state.
 *
 * Contains labor force, job counts, and employment metrics:
 * - Working-age beings and labor force participation
 * - Employment/unemployment counts
 * - Job distribution by sector (exchange = commercial, fabrication = industrial)
 * - Unemployment rate and labor participation rate
 * - Commute satisfaction index
 *
 * Target size: ~45 bytes.
 */
struct EmploymentData {
    /// Population counts
    uint32_t working_age_beings = 0;    ///< Total working-age population
    uint32_t labor_force = 0;           ///< Beings actively in labor force
    uint32_t employed_laborers = 0;     ///< Currently employed beings
    uint32_t unemployed = 0;            ///< Unemployed beings seeking work

    /// Job counts
    uint32_t total_jobs = 0;            ///< Total available jobs in city
    uint32_t exchange_jobs = 0;         ///< Commercial sector jobs
    uint32_t fabrication_jobs = 0;      ///< Industrial sector jobs

    /// Rates (0-100 scale)
    uint8_t unemployment_rate = 0;      ///< Percentage unemployed (0-100)
    uint8_t labor_participation = 65;   ///< Labor force participation rate (0-100)

    /// Per-sector employed counts
    uint32_t exchange_employed = 0;     ///< Employed in commercial sector
    uint32_t fabrication_employed = 0;  ///< Employed in industrial sector

    /// Quality metric
    uint8_t avg_commute_satisfaction = 50;  ///< Average commute satisfaction (0-100)
};

static_assert(sizeof(EmploymentData) <= 52, "EmploymentData should be approximately 45 bytes");

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_EMPLOYMENTDATA_H
