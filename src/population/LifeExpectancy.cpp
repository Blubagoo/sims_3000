/**
 * @file LifeExpectancy.cpp
 * @brief Implementation of life expectancy calculation (Ticket E10-028)
 */

#include "sims3000/population/LifeExpectancy.h"
#include <algorithm>

namespace sims3000 {
namespace population {

/**
 * @brief Linear interpolation helper
 */
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Clamp value to range
 */
static inline float clamp(float value, float min_val, float max_val) {
    return std::max(min_val, std::min(value, max_val));
}

LifeExpectancyResult calculate_life_expectancy(const LifeExpectancyInput& input) {
    LifeExpectancyResult result{};

    // Normalize indices to 0-1 range
    float health_t = input.health_index / 100.0f;
    float contamination_t = input.contamination_level / 100.0f;
    float disorder_t = input.disorder_level / 100.0f;
    float education_t = input.education_index / 100.0f;
    float harmony_t = input.harmony_index / 100.0f;

    // Calculate modifiers
    // Health: better health increases life expectancy (0.7 - 1.3)
    result.health_modifier = lerp(0.7f, 1.3f, health_t);

    // Contamination: INVERTED - higher contamination reduces life expectancy (1.0 - 0.6)
    result.contamination_modifier = lerp(1.0f, 0.6f, contamination_t);

    // Disorder: INVERTED - higher disorder reduces life expectancy (1.0 - 0.9)
    result.disorder_modifier = lerp(1.0f, 0.9f, disorder_t);

    // Education: better education increases life expectancy (0.95 - 1.1)
    result.education_modifier = lerp(0.95f, 1.1f, education_t);

    // Harmony: better harmony increases life expectancy (0.9 - 1.1)
    result.harmony_modifier = lerp(0.9f, 1.1f, harmony_t);

    // Combine all modifiers
    float combined_modifier = result.health_modifier
                            * result.contamination_modifier
                            * result.disorder_modifier
                            * result.education_modifier
                            * result.harmony_modifier;

    // Calculate final life expectancy
    float raw_expectancy = BASE_LIFE_EXPECTANCY * combined_modifier;

    // Clamp to valid range
    result.life_expectancy = clamp(raw_expectancy, MIN_LIFE_EXPECTANCY, MAX_LIFE_EXPECTANCY);

    return result;
}

} // namespace population
} // namespace sims3000
