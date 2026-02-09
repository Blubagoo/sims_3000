/**
 * @file MigrationOut.h
 * @brief Migration out calculation (Ticket E10-026)
 *
 * Calculates outbound migration based on city desperation factors:
 * high disorder, contamination, low job availability, low harmony.
 * Rate is capped and never causes total exodus.
 */

#ifndef SIMS3000_POPULATION_MIGRATIONOUT_H
#define SIMS3000_POPULATION_MIGRATIONOUT_H

#include <cstdint>
#include "sims3000/population/MigrationFactors.h"

namespace sims3000 {
namespace population {

namespace constants {
    constexpr float BASE_OUT_RATE = 0.005f;             ///< 0.5% per cycle base departure rate
    constexpr float MAX_OUT_RATE = 0.05f;               ///< 5% cap on departure rate
    constexpr float DESPERATION_THRESHOLD = 50.0f;      ///< Threshold for desperation triggers
} // namespace constants

/**
 * @struct MigrationOutResult
 * @brief Result of outbound migration calculation.
 *
 * Contains the number of departing migrants and intermediate
 * calculation values for debugging/display.
 */
struct MigrationOutResult {
    uint32_t migrants_out;            ///< Number of beings migrating out
    float effective_out_rate;         ///< Effective departure rate applied
    float desperation_factor;         ///< Accumulated desperation score
};

/**
 * @brief Calculate outbound migration for a city.
 *
 * Algorithm:
 * - Accumulate desperation from: disorder > 50, contamination > 50,
 *   job_availability < 30, harmony < 30
 * - effective_rate = BASE_OUT_RATE + desperation * 0.05
 * - Cap at MAX_OUT_RATE (5%)
 * - migrants_out = round(total_beings * effective_rate)
 * - Never cause total exodus: min(migrants_out, total_beings - 1)
 *
 * @param factors Migration factors for the city
 * @param total_beings Current total population
 * @return MigrationOutResult with migrant count and calculation details
 */
MigrationOutResult calculate_migration_out(const MigrationFactors& factors,
                                             uint32_t total_beings);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_MIGRATIONOUT_H
