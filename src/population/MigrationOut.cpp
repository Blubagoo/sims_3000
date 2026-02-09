/**
 * @file MigrationOut.cpp
 * @brief Migration out calculation implementation (Ticket E10-026)
 *
 * Computes outbound migration based on accumulated desperation from
 * negative city conditions: disorder, contamination, low jobs, low harmony.
 */

#include "sims3000/population/MigrationOut.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

MigrationOutResult calculate_migration_out(const MigrationFactors& factors,
                                             uint32_t total_beings) {
    MigrationOutResult result{};

    // Accumulate desperation from various negative conditions
    float desperation = 0.0f;

    if (factors.disorder_level > 50) {
        desperation += (static_cast<float>(factors.disorder_level) - 50.0f) / 100.0f;
    }
    if (factors.contamination_level > 50) {
        desperation += (static_cast<float>(factors.contamination_level) - 50.0f) / 100.0f;
    }
    if (factors.job_availability < 30) {
        desperation += (30.0f - static_cast<float>(factors.job_availability)) / 100.0f;
    }
    if (factors.harmony_level < 30) {
        desperation += (30.0f - static_cast<float>(factors.harmony_level)) / 100.0f;
    }

    result.desperation_factor = desperation;

    // Effective out rate: base + desperation contribution, capped
    float effective_rate = constants::BASE_OUT_RATE + desperation * 0.05f;
    effective_rate = std::min(effective_rate, constants::MAX_OUT_RATE);
    result.effective_out_rate = effective_rate;

    // Calculate migrants out
    result.migrants_out = static_cast<uint32_t>(
        std::round(static_cast<float>(total_beings) * effective_rate));

    // Never cause total exodus: leave at least 1 being
    if (total_beings > 1 && result.migrants_out >= total_beings) {
        result.migrants_out = total_beings - 1;
    }

    return result;
}

} // namespace population
} // namespace sims3000
