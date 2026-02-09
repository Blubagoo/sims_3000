/**
 * @file MigrationApplication.cpp
 * @brief Implementation of migration application (Ticket E10-027)
 */

#include "sims3000/population/MigrationApplication.h"
#include "sims3000/population/MigrationIn.h"
#include "sims3000/population/MigrationOut.h"
#include <algorithm>

namespace sims3000 {
namespace population {

MigrationEvent apply_migration(PopulationData& data,
                                const MigrationFactors& factors,
                                uint32_t available_housing) {
    // Calculate migration in using net_attraction and available housing
    MigrationInResult in_result = calculate_migration_in(
        factors.net_attraction,
        data.total_beings,
        available_housing
    );

    // Calculate migration out using factors and current population
    MigrationOutResult out_result = calculate_migration_out(
        factors,
        data.total_beings
    );

    // Extract migrant counts
    uint32_t migrants_in = in_result.migrants_in;
    uint32_t migrants_out = out_result.migrants_out;

    // Apply migrants_in (already capped by available_housing in calculate_migration_in)
    data.total_beings += migrants_in;

    // Apply migrants_out (ensure we never go below 0)
    if (migrants_out > data.total_beings) {
        migrants_out = data.total_beings;
    }
    data.total_beings -= migrants_out;

    // Update net_migration statistic
    data.net_migration = static_cast<int32_t>(migrants_in) - static_cast<int32_t>(migrants_out);

    // Update growth_rate (natural + migration)
    // growth_rate is the combined effect of natural growth and migration
    data.growth_rate = static_cast<float>(data.natural_growth + data.net_migration);

    // Return event for logging/UI
    return MigrationEvent{
        migrants_in,
        migrants_out,
        data.net_migration
    };
}

} // namespace population
} // namespace sims3000
