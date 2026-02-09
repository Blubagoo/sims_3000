/**
 * @file MigrationApplication.h
 * @brief Migration application to population (Ticket E10-027)
 *
 * Applies net migration to population by combining migration in and out
 * calculations. Updates total_beings, net_migration, and growth_rate.
 * Emits a MigrationEvent for logging/UI display.
 */

#ifndef SIMS3000_POPULATION_MIGRATIONAPPLICATION_H
#define SIMS3000_POPULATION_MIGRATIONAPPLICATION_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/MigrationFactors.h"

namespace sims3000 {
namespace population {

/**
 * @struct MigrationEvent
 * @brief Event emitted after migration is applied to population.
 *
 * Contains the migration flow numbers for this cycle:
 * - migrants_in: Number of beings who moved in
 * - migrants_out: Number of beings who moved out
 * - net_migration: Net change (in - out)
 */
struct MigrationEvent {
    uint32_t migrants_in;     ///< Number of beings migrating in this cycle
    uint32_t migrants_out;    ///< Number of beings migrating out this cycle
    int32_t net_migration;    ///< Net migration (in - out, can be negative)
};

/**
 * @brief Apply migration to population data.
 *
 * Algorithm:
 * 1. Call calculate_migration_in() with net_attraction, total_beings, available_housing
 * 2. Call calculate_migration_out() with factors, total_beings
 * 3. Apply migrants_in to total_beings (capped by available_housing)
 * 4. Apply migrants_out from total_beings (never below 0)
 * 5. Update net_migration = migrants_in - migrants_out
 * 6. Update growth_rate = natural_growth + net_migration
 * 7. Return MigrationEvent with in/out/net numbers
 *
 * Should be called every 20 ticks (1 second game time) per spec.
 *
 * @param data Mutable reference to PopulationData (will update total_beings, net_migration, growth_rate)
 * @param factors Migration factors for the city (net_attraction, disorder, etc.)
 * @param available_housing Available housing capacity (max_capacity - total_beings)
 * @return MigrationEvent with migration statistics
 */
MigrationEvent apply_migration(PopulationData& data,
                                const MigrationFactors& factors,
                                uint32_t available_housing);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_MIGRATIONAPPLICATION_H
