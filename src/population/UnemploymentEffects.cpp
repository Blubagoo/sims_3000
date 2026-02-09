/**
 * @file UnemploymentEffects.cpp
 * @brief Implementation of unemployment effects (Ticket E10-023)
 */

#include "sims3000/population/UnemploymentEffects.h"
#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

UnemploymentEffectResult calculate_unemployment_effect(float unemployment_rate) {
    UnemploymentEffectResult result;

    // Check for full employment (unemployment <= 2%)
    if (unemployment_rate <= 2.0f) {
        result.harmony_modifier = FULL_EMPLOYMENT_BONUS;
        result.is_full_employment = true;
    } else {
        // Apply penalty based on unemployment rate
        float penalty = unemployment_rate * UNEMPLOYMENT_HARMONY_PENALTY_RATE;

        // Cap at maximum penalty
        penalty = std::min(penalty, MAX_UNEMPLOYMENT_PENALTY);

        result.harmony_modifier = -penalty;
        result.is_full_employment = false;
    }

    return result;
}

void apply_unemployment_effect(PopulationData& pop, float unemployment_rate) {
    UnemploymentEffectResult effect = calculate_unemployment_effect(unemployment_rate);

    // Apply modifier to harmony index
    float new_harmony = static_cast<float>(pop.harmony_index) + effect.harmony_modifier;

    // Clamp to valid range [0, 100]
    new_harmony = std::max(0.0f, std::min(100.0f, new_harmony));

    pop.harmony_index = static_cast<uint8_t>(std::round(new_harmony));
}

} // namespace population
} // namespace sims3000
