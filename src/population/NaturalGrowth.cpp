/**
 * @file NaturalGrowth.cpp
 * @brief Natural population growth implementation (Ticket E10-017)
 *
 * Combines birth and death rate calculations to compute and apply
 * natural growth to a PopulationData instance.
 */

#include "sims3000/population/NaturalGrowth.h"
#include "sims3000/population/BirthRateCalculation.h"
#include "sims3000/population/DeathRateCalculation.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

NaturalGrowthResult apply_natural_growth(PopulationData& pop,
                                          uint32_t available_housing,
                                          float contamination_level,
                                          float service_coverage) {
    NaturalGrowthResult result{};

    // Calculate births using birth rate calculation
    BirthRateResult birth_result = calculate_birth_rate(pop, available_housing);
    result.births = birth_result.births;

    // Calculate deaths using death rate calculation
    DeathRateResult death_result = calculate_death_rate(pop, contamination_level, service_coverage);
    result.deaths = death_result.deaths;

    // Natural growth = births - deaths
    result.natural_growth = static_cast<int32_t>(result.births) - static_cast<int32_t>(result.deaths);

    // New total: never go below zero
    int64_t new_total = static_cast<int64_t>(pop.total_beings) + static_cast<int64_t>(result.natural_growth);
    if (new_total < 0) {
        new_total = 0;
    }
    result.new_total_beings = static_cast<uint32_t>(new_total);

    // Update PopulationData in-place
    pop.total_beings = result.new_total_beings;
    pop.natural_growth = result.natural_growth;
    pop.birth_rate_per_1000 = static_cast<uint16_t>(std::round(birth_result.effective_rate * 1000.0f));
    pop.death_rate_per_1000 = static_cast<uint16_t>(std::round(death_result.effective_rate * 1000.0f));

    return result;
}

} // namespace population
} // namespace sims3000
