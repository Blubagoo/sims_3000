/**
 * @file DeathRateCalculation.cpp
 * @brief Death rate calculation implementation (Ticket E10-016)
 *
 * Computes death rate from population health, contamination,
 * service coverage, and age distribution.
 * Uses linear interpolation for modifier calculations.
 */

#include "sims3000/population/DeathRateCalculation.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

namespace {

/// Linear interpolation helper
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // anonymous namespace

DeathRateResult calculate_death_rate(const PopulationData& pop,
                                     float contamination_level,
                                     float service_coverage) {
    DeathRateResult result{};

    // Zero population means zero deaths
    if (pop.total_beings == 0) {
        result.effective_rate = 0.0f;
        result.health_modifier = 0.0f;
        result.contamination_modifier = 0.0f;
        result.services_modifier = 0.0f;
        result.age_modifier = 0.0f;
        result.deaths = 0;
        return result;
    }

    // Health modifier: worse health (lower index) = more deaths
    // Inverted: 1.0 - (health/100) so low health gives high t
    result.health_modifier = lerp(
        constants::HEALTH_DEATH_MIN,
        constants::HEALTH_DEATH_MAX,
        1.0f - (pop.health_index / 100.0f)
    );

    // Contamination modifier: higher contamination = more deaths
    result.contamination_modifier = lerp(
        constants::CONTAMINATION_DEATH_MIN,
        constants::CONTAMINATION_DEATH_MAX,
        contamination_level / 100.0f
    );

    // Services modifier: lower coverage = more deaths
    // Inverted: 1.0 - (coverage/100) so low coverage gives high t
    result.services_modifier = lerp(
        constants::SERVICES_DEATH_MIN,
        constants::SERVICES_DEATH_MAX,
        1.0f - (service_coverage / 100.0f)
    );

    // Age modifier: more elders = more deaths
    result.age_modifier = lerp(
        constants::AGE_DEATH_MIN,
        constants::AGE_DEATH_MAX,
        pop.elder_percent / 100.0f
    );

    // Effective rate = base * all modifiers
    result.effective_rate = constants::BASE_DEATH_RATE
        * result.health_modifier
        * result.contamination_modifier
        * result.services_modifier
        * result.age_modifier;

    // Calculate actual deaths: round(total_beings * effective_rate)
    float raw_deaths = static_cast<float>(pop.total_beings) * result.effective_rate;
    uint32_t rounded_deaths = static_cast<uint32_t>(std::round(raw_deaths));

    // Cap deaths at MAX_DEATH_PERCENT of total population
    uint32_t max_deaths = static_cast<uint32_t>(
        std::round(constants::MAX_DEATH_PERCENT * static_cast<float>(pop.total_beings))
    );
    result.deaths = std::min(rounded_deaths, max_deaths);

    return result;
}

} // namespace population
} // namespace sims3000
