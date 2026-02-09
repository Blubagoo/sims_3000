/**
 * @file BirthRateCalculation.h
 * @brief Birth rate calculation for population simulation (Ticket E10-015)
 *
 * Pure calculation function that computes birth rate based on population
 * metrics and available housing. No side effects.
 */

#ifndef SIMS3000_POPULATION_BIRTHRATECALCULATION_H
#define SIMS3000_POPULATION_BIRTHRATECALCULATION_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"

namespace sims3000 {
namespace population {

namespace constants {
    constexpr float BASE_BIRTH_RATE = 0.015f;       ///< 15 per 1000 per cycle
    constexpr float HARMONY_BIRTH_MIN = 0.5f;       ///< Min harmony modifier
    constexpr float HARMONY_BIRTH_MAX = 1.5f;       ///< Max harmony modifier
    constexpr float HEALTH_BIRTH_MIN = 0.7f;        ///< Min health modifier
    constexpr float HEALTH_BIRTH_MAX = 1.2f;        ///< Max health modifier
    constexpr float HOUSING_BIRTH_MIN = 0.1f;       ///< Min housing modifier
    constexpr float HOUSING_BIRTH_MAX = 1.0f;       ///< Max housing modifier
} // namespace constants

/**
 * @struct BirthRateResult
 * @brief Result of birth rate calculation.
 *
 * Contains the effective rate, individual modifiers, and actual birth count.
 */
struct BirthRateResult {
    float effective_rate;       ///< Final rate per 1000
    float harmony_modifier;    ///< Harmony contribution to rate
    float health_modifier;     ///< Health contribution to rate
    float housing_modifier;    ///< Housing contribution to rate
    uint32_t births;           ///< Actual births this cycle
};

/**
 * @brief Calculate birth rate for a population.
 *
 * Pure calculation function with no side effects. Computes births based on:
 * - Base birth rate modified by harmony, health, and housing factors
 * - Minimum 1 birth if population > 0 and housing is available
 * - Zero births if population is 0 or no housing is available
 *
 * @param pop Population data for the city
 * @param available_housing Number of available housing slots
 * @return BirthRateResult with effective rate, modifiers, and birth count
 */
BirthRateResult calculate_birth_rate(const PopulationData& pop, uint32_t available_housing);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_BIRTHRATECALCULATION_H
