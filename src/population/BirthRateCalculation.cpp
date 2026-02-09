/**
 * @file BirthRateCalculation.cpp
 * @brief Birth rate calculation implementation (Ticket E10-015)
 *
 * Computes birth rate from population state and available housing.
 * Uses linear interpolation for modifier calculations.
 */

#include "sims3000/population/BirthRateCalculation.h"

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

BirthRateResult calculate_birth_rate(const PopulationData& pop, uint32_t available_housing) {
    BirthRateResult result{};

    // Zero population or zero housing means zero births
    if (pop.total_beings == 0 || available_housing == 0) {
        result.effective_rate = 0.0f;
        result.harmony_modifier = 0.0f;
        result.health_modifier = 0.0f;
        result.housing_modifier = 0.0f;
        result.births = 0;
        return result;
    }

    // Calculate modifiers via linear interpolation
    result.harmony_modifier = lerp(
        constants::HARMONY_BIRTH_MIN,
        constants::HARMONY_BIRTH_MAX,
        pop.harmony_index / 100.0f
    );

    result.health_modifier = lerp(
        constants::HEALTH_BIRTH_MIN,
        constants::HEALTH_BIRTH_MAX,
        pop.health_index / 100.0f
    );

    // Housing ratio: available_housing / total_beings, clamped to [0, 1]
    float housing_ratio = std::min(
        1.0f,
        static_cast<float>(available_housing) / static_cast<float>(std::max(1u, pop.total_beings))
    );
    result.housing_modifier = lerp(
        constants::HOUSING_BIRTH_MIN,
        constants::HOUSING_BIRTH_MAX,
        housing_ratio
    );

    // Effective rate = base * all modifiers
    result.effective_rate = constants::BASE_BIRTH_RATE
        * result.harmony_modifier
        * result.health_modifier
        * result.housing_modifier;

    // Calculate actual births: round(total_beings * effective_rate), minimum 1
    float raw_births = static_cast<float>(pop.total_beings) * result.effective_rate;
    uint32_t rounded_births = static_cast<uint32_t>(std::round(raw_births));
    result.births = std::max(1u, rounded_births);

    return result;
}

} // namespace population
} // namespace sims3000
