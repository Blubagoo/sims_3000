/**
 * @file MigrationIn.cpp
 * @brief Migration in calculation implementation (Ticket E10-025)
 *
 * Computes inbound migration based on city attractiveness, colony size,
 * and housing availability.
 */

#include "sims3000/population/MigrationIn.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

MigrationInResult calculate_migration_in(int8_t net_attraction,
                                           uint32_t total_beings,
                                           uint32_t available_housing) {
    MigrationInResult result{};

    // Too unattractive: no one wants to move in
    if (net_attraction < -50) {
        result.migrants_in = 0;
        result.attraction_multiplier = 0.0f;
        result.colony_size_bonus = total_beings * constants::COLONY_SIZE_FACTOR;
        return result;
    }

    // Normalize net_attraction from [-100, +100] to [0, 1]
    float attraction_normalized = (static_cast<float>(net_attraction) + 100.0f) / 200.0f;

    // Lerp attraction multiplier between MIN (0) and MAX (2)
    result.attraction_multiplier =
        constants::ATTRACTION_MULTIPLIER_MIN +
        attraction_normalized * (constants::ATTRACTION_MULTIPLIER_MAX - constants::ATTRACTION_MULTIPLIER_MIN);

    // Colony size bonus: larger cities attract more
    result.colony_size_bonus = static_cast<float>(total_beings) * constants::COLONY_SIZE_FACTOR;

    // Raw migration calculation
    float raw_migration = (static_cast<float>(constants::BASE_MIGRATION) + result.colony_size_bonus)
                          * result.attraction_multiplier;

    // Cap by available housing
    uint32_t migration_count = static_cast<uint32_t>(std::round(raw_migration));
    result.migrants_in = std::min(migration_count, available_housing);

    return result;
}

} // namespace population
} // namespace sims3000
