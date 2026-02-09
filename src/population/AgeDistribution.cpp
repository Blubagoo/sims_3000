/**
 * @file AgeDistribution.cpp
 * @brief Implementation of age distribution evolution (Ticket E10-018)
 */

#include "sims3000/population/AgeDistribution.h"
#include <algorithm>

namespace sims3000 {
namespace population {

AgeDistributionResult evolve_age_distribution(
    uint8_t current_youth,
    uint8_t current_adult,
    uint8_t current_elder,
    uint32_t births,
    uint32_t deaths,
    uint32_t total_beings
) {
    // Edge case: if population is zero or becomes zero, return default distribution
    if (total_beings == 0) {
        return AgeDistributionResult{33, 34, 33};
    }

    // Step 1: Convert percentages to counts
    int32_t youth_count = (total_beings * current_youth) / 100;
    int32_t adult_count = (total_beings * current_adult) / 100;
    int32_t elder_count = (total_beings * current_elder) / 100;

    // Step 2: Apply aging transitions
    int32_t youth_aging = static_cast<int32_t>(youth_count * YOUTH_TO_ADULT_RATE);
    int32_t adult_aging = static_cast<int32_t>(adult_count * ADULT_TO_ELDER_RATE);

    youth_count -= youth_aging;
    adult_count += youth_aging;
    adult_count -= adult_aging;
    elder_count += adult_aging;

    // Step 3: Apply births to youth
    youth_count += static_cast<int32_t>(births);

    // Step 4: Apply weighted deaths, never allowing negative counts
    int32_t youth_deaths = static_cast<int32_t>(deaths * YOUTH_DEATH_WEIGHT);
    int32_t adult_deaths = static_cast<int32_t>(deaths * ADULT_DEATH_WEIGHT);
    int32_t elder_deaths = static_cast<int32_t>(deaths * ELDER_DEATH_WEIGHT);

    youth_count = std::max(0, youth_count - youth_deaths);
    adult_count = std::max(0, adult_count - adult_deaths);
    elder_count = std::max(0, elder_count - elder_deaths);

    // Step 5: Recompute total after all changes
    int32_t new_total = youth_count + adult_count + elder_count;

    // If total is zero, return default distribution
    if (new_total == 0) {
        return AgeDistributionResult{33, 34, 33};
    }

    // Convert to percentages (integer division)
    uint8_t youth_pct = static_cast<uint8_t>((youth_count * 100) / new_total);
    uint8_t adult_pct = static_cast<uint8_t>((adult_count * 100) / new_total);
    uint8_t elder_pct = static_cast<uint8_t>((elder_count * 100) / new_total);

    // Ensure percentages sum to exactly 100 by allocating remainder to largest group
    int32_t sum = youth_pct + adult_pct + elder_pct;
    int32_t remainder = 100 - sum;

    if (remainder != 0) {
        // Find which group is largest and add the remainder to it
        if (youth_count >= adult_count && youth_count >= elder_count) {
            youth_pct += remainder;
        } else if (adult_count >= elder_count) {
            adult_pct += remainder;
        } else {
            elder_pct += remainder;
        }
    }

    return AgeDistributionResult{youth_pct, adult_pct, elder_pct};
}

} // namespace population
} // namespace sims3000
