/**
 * @file LifeExpectancy.h
 * @brief Life expectancy calculation for UI display (Ticket E10-028)
 *
 * Calculates life expectancy as a derived statistic based on:
 * - Health index (0.7x - 1.3x)
 * - Contamination level (0.6x - 1.0x, inverted)
 * - Disorder level (0.9x - 1.0x, inverted)
 * - Education index (0.95x - 1.1x)
 * - Harmony index (0.9x - 1.1x)
 *
 * Base life expectancy: 75 cycles
 * Range: 30-120 cycles
 */

#ifndef SIMS3000_POPULATION_LIFEEXPECTANCY_H
#define SIMS3000_POPULATION_LIFEEXPECTANCY_H

#include <cstdint>

namespace sims3000 {
namespace population {

/// Base life expectancy in simulation cycles
constexpr float BASE_LIFE_EXPECTANCY = 75.0f;

/// Minimum life expectancy
constexpr float MIN_LIFE_EXPECTANCY = 30.0f;

/// Maximum life expectancy
constexpr float MAX_LIFE_EXPECTANCY = 120.0f;

/**
 * @struct LifeExpectancyInput
 * @brief Input parameters for life expectancy calculation
 */
struct LifeExpectancyInput {
    uint8_t health_index;          ///< 0-100 health quality
    uint8_t contamination_level;   ///< 0-100 average city contamination
    uint8_t disorder_level;        ///< 0-100 average city disorder
    uint8_t education_index;       ///< 0-100 education quality
    uint8_t harmony_index;         ///< 0-100 social harmony
};

/**
 * @struct LifeExpectancyResult
 * @brief Calculated life expectancy with modifiers breakdown
 */
struct LifeExpectancyResult {
    float life_expectancy;          ///< Final life expectancy (30-120 cycles)
    float health_modifier;          ///< Health contribution (0.7-1.3)
    float contamination_modifier;   ///< Contamination contribution (0.6-1.0)
    float disorder_modifier;        ///< Disorder contribution (0.9-1.0)
    float education_modifier;       ///< Education contribution (0.95-1.1)
    float harmony_modifier;         ///< Harmony contribution (0.9-1.1)
};

/**
 * @brief Calculate life expectancy based on city conditions
 *
 * Formula:
 * - health_modifier = lerp(0.7, 1.3, health_index/100)
 * - contamination_modifier = lerp(1.0, 0.6, contamination_level/100)  [INVERTED]
 * - disorder_modifier = lerp(1.0, 0.9, disorder_level/100)  [INVERTED]
 * - education_modifier = lerp(0.95, 1.1, education_index/100)
 * - harmony_modifier = lerp(0.9, 1.1, harmony_index/100)
 * - life_expectancy = BASE * health * contam * disorder * education * harmony
 * - Clamp to [30, 120]
 *
 * @param input City condition indices
 * @return Life expectancy result with modifiers breakdown
 */
LifeExpectancyResult calculate_life_expectancy(const LifeExpectancyInput& input);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_LIFEEXPECTANCY_H
