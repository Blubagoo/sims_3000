/**
 * @file HealthIndex.cpp
 * @brief Health index calculation implementation (Ticket E10-029)
 */

#include <sims3000/population/HealthIndex.h>
#include <algorithm>

namespace sims3000 {
namespace population {

HealthResult calculate_health_index(const HealthInput& input) {
    HealthResult result{};

    // Medical coverage modifier: (coverage - 0.5) * 2 * MAX
    // At 0.0 coverage → -25, at 0.5 → 0, at 1.0 → +25
    result.medical_modifier = (input.medical_coverage - 0.5f) * 2.0f * MEDICAL_MODIFIER_MAX;

    // Contamination modifier: -level * MAX
    // At 0.0 contamination → 0, at 1.0 → -30
    result.contamination_modifier = -input.contamination_level * CONTAMINATION_MODIFIER_MAX;

    // Fluid modifier: if has fluid, scale by coverage; else -MAX
    // With fluid: at 0.0 coverage → -10, at 0.5 → 0, at 1.0 → +10
    // Without fluid: -10
    if (input.has_fluid) {
        result.fluid_modifier = (input.fluid_coverage - 0.5f) * 2.0f * FLUID_MODIFIER_MAX;
    } else {
        result.fluid_modifier = -FLUID_MODIFIER_MAX;
    }

    // Calculate final health index
    float health_float = static_cast<float>(BASE_HEALTH)
                       + result.medical_modifier
                       + result.contamination_modifier
                       + result.fluid_modifier;

    // Clamp to [0, 100]
    result.health_index = static_cast<uint8_t>(
        std::clamp(health_float, 0.0f, 100.0f)
    );

    return result;
}

void apply_health_index(PopulationData& pop, const HealthInput& input) {
    HealthResult result = calculate_health_index(input);
    pop.health_index = result.health_index;
}

} // namespace population
} // namespace sims3000
