/**
 * @file MigrationIn.h
 * @brief Migration in calculation (Ticket E10-025)
 *
 * Calculates inbound migration based on city attractiveness,
 * colony size, and available housing. More attractive and larger
 * cities draw more migrants, capped by housing availability.
 */

#ifndef SIMS3000_POPULATION_MIGRATIONIN_H
#define SIMS3000_POPULATION_MIGRATIONIN_H

#include <cstdint>

namespace sims3000 {
namespace population {

namespace constants {
    constexpr uint32_t BASE_MIGRATION = 50;             ///< Beings per cycle at neutral attraction
    constexpr float ATTRACTION_MULTIPLIER_MIN = 0.0f;   ///< Multiplier at minimum attraction
    constexpr float ATTRACTION_MULTIPLIER_MAX = 2.0f;   ///< Multiplier at maximum attraction
    constexpr float COLONY_SIZE_FACTOR = 0.001f;        ///< Larger colonies attract more
} // namespace constants

/**
 * @struct MigrationInResult
 * @brief Result of inbound migration calculation.
 *
 * Contains the number of incoming migrants and intermediate
 * calculation values for debugging/display.
 */
struct MigrationInResult {
    uint32_t migrants_in;              ///< Number of beings migrating in
    float attraction_multiplier;       ///< Attraction multiplier applied (0-2)
    float colony_size_bonus;           ///< Bonus from colony size
};

/**
 * @brief Calculate inbound migration for a city.
 *
 * Algorithm:
 * - Normalize net_attraction from [-100, +100] to [0, 1]
 * - Lerp attraction multiplier between MIN (0) and MAX (2)
 * - Add colony size bonus = total_beings * COLONY_SIZE_FACTOR
 * - raw_migration = (BASE_MIGRATION + colony_size_bonus) * attraction_mult
 * - Cap by available_housing
 * - If net_attraction < -50, migrants_in = 0 (too unattractive)
 *
 * @param net_attraction City attractiveness score (-100 to +100)
 * @param total_beings Current total population
 * @param available_housing Available housing capacity
 * @return MigrationInResult with migrant count and calculation details
 */
MigrationInResult calculate_migration_in(int8_t net_attraction,
                                           uint32_t total_beings,
                                           uint32_t available_housing);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_MIGRATIONIN_H
