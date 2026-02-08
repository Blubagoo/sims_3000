/**
 * @file MigrationEffects.cpp
 * @brief Population migration calculation implementation (Epic 8, Ticket E8-024)
 *
 * Implements inbound/outbound migration formulas:
 * - Immigration: migration_capacity * demand_factor * harmony_factor (capped per cycle)
 * - Emigration:  migration_capacity * (disorder_index / 100) * tribute_penalty
 */

#include <sims3000/port/MigrationEffects.h>
#include <algorithm>

namespace sims3000 {
namespace port {

MigrationResult calculate_migration(const MigrationInput& input) {
    MigrationResult result;

    // Calculate per-cycle immigration cap
    result.max_immigration = static_cast<int32_t>(
        10 + (input.external_connection_count * 5));

    // Clamp input factors to valid ranges
    float demand = std::max(0.5f, std::min(input.demand_factor, 1.5f));
    float harmony = std::max(0.0f, std::min(input.harmony_factor, 1.0f));
    float disorder = std::max(0.0f, std::min(input.disorder_index, 100.0f));
    float tribute = std::max(1.0f, input.tribute_penalty);

    // Inbound migration: migration_capacity * demand_factor * harmony_factor
    float raw_immigration = static_cast<float>(input.total_migration_capacity)
                          * demand * harmony;
    int32_t uncapped_immigration = static_cast<int32_t>(raw_immigration);

    // Cap immigration at max per cycle
    result.immigration_rate = std::min(uncapped_immigration, result.max_immigration);

    // Outbound migration: migration_capacity * (disorder_index / 100) * tribute_penalty
    float raw_emigration = static_cast<float>(input.total_migration_capacity)
                         * (disorder / 100.0f) * tribute;
    result.emigration_rate = static_cast<int32_t>(raw_emigration);

    // Net migration
    result.net_migration = result.immigration_rate - result.emigration_rate;

    return result;
}

} // namespace port
} // namespace sims3000
