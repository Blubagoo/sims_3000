/**
 * @file UnemploymentEffects.h
 * @brief Unemployment effects on harmony (Ticket E10-023)
 *
 * Calculates and applies harmony index modifications based on
 * unemployment rate. Full employment provides bonuses, while
 * high unemployment penalizes harmony.
 */

#ifndef SIMS3000_POPULATION_UNEMPLOYMENTEFFECTS_H
#define SIMS3000_POPULATION_UNEMPLOYMENTEFFECTS_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"

namespace sims3000 {
namespace population {

/// Harmony penalty rate per 1% unemployment (-0.5 harmony per 1%)
constexpr float UNEMPLOYMENT_HARMONY_PENALTY_RATE = 0.5f;

/// Harmony bonus for full employment (+5 harmony at 0-2% unemployment)
constexpr float FULL_EMPLOYMENT_BONUS = 5.0f;

/// Maximum penalty cap for high unemployment (-30 harmony maximum)
constexpr float MAX_UNEMPLOYMENT_PENALTY = 30.0f;

/**
 * @struct UnemploymentEffectResult
 * @brief Result of unemployment effect calculation.
 *
 * Contains the harmony modifier to apply and whether
 * the city has achieved full employment status.
 */
struct UnemploymentEffectResult {
    float harmony_modifier;      ///< Change to apply to harmony_index (can be negative)
    bool is_full_employment;     ///< True if unemployment <= 2%
};

/**
 * @brief Calculate harmony impact from unemployment.
 *
 * Algorithm:
 * - If unemployment_rate <= 2%: modifier = +FULL_EMPLOYMENT_BONUS
 * - Else: modifier = -(unemployment_rate * PENALTY_RATE), capped at -MAX_UNEMPLOYMENT_PENALTY
 *
 * @param unemployment_rate Unemployment rate as percentage (0.0-100.0)
 * @return Unemployment effect with harmony modifier and full employment flag
 */
UnemploymentEffectResult calculate_unemployment_effect(float unemployment_rate);

/**
 * @brief Apply unemployment effect to PopulationData.
 *
 * Modifies the harmony_index field in place, clamping result to [0, 100].
 *
 * @param pop PopulationData to modify (modified in place)
 * @param unemployment_rate Unemployment rate as percentage (0.0-100.0)
 */
void apply_unemployment_effect(PopulationData& pop, float unemployment_rate);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_UNEMPLOYMENTEFFECTS_H
