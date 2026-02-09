/**
 * @file MigrationFactors.h
 * @brief Migration attractiveness factors (Ticket E10-013)
 *
 * Per-player component tracking factors that influence
 * migration into and out of the city. Positive factors
 * attract new beings; negative factors repel them.
 */

#ifndef SIMS3000_POPULATION_MIGRATIONFACTORS_H
#define SIMS3000_POPULATION_MIGRATIONFACTORS_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @struct MigrationFactors
 * @brief Factors influencing city migration attractiveness.
 *
 * Positive factors (0-100, higher = more attractive):
 * - Job availability, housing, sector value, services, harmony
 *
 * Negative factors (0-100, higher = worse):
 * - Disorder, contamination, tribute burden, congestion
 *
 * Computed results:
 * - net_attraction: weighted sum of factors (-100 to +100)
 * - migration_pressure: final migration pressure (-100 to +100)
 *
 * Target size: ~12 bytes.
 */
struct MigrationFactors {
    /// Positive factors (0-100 each, higher = more attractive)
    uint8_t job_availability = 50;    ///< Jobs available relative to labor force
    uint8_t housing_availability = 50; ///< Housing vacancies relative to demand
    uint8_t sector_value_avg = 50;    ///< Average land/property value
    uint8_t service_coverage = 50;    ///< City services coverage level
    uint8_t harmony_level = 50;       ///< Social harmony / happiness

    /// Negative factors (0-100 each, higher = worse)
    uint8_t disorder_level = 0;       ///< Crime / disorder level
    uint8_t contamination_level = 0;  ///< Pollution / contamination
    uint8_t tribute_burden = 0;       ///< Tax burden on citizens
    uint8_t congestion_level = 0;     ///< Traffic / infrastructure congestion

    /// Computed migration metrics
    int8_t net_attraction = 0;        ///< Net city attractiveness (-100 to +100)
    int8_t migration_pressure = 0;    ///< Final migration pressure (-100 to +100)
};

static_assert(sizeof(MigrationFactors) == 11, "MigrationFactors should be approximately 12 bytes");

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_MIGRATIONFACTORS_H
