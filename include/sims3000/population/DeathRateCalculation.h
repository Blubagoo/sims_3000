/**
 * @file DeathRateCalculation.h
 * @brief Death rate calculation for population simulation (Ticket E10-016)
 *
 * Pure calculation function that computes death rate based on population
 * health, contamination, service coverage, and age distribution.
 * No side effects.
 */

#ifndef SIMS3000_POPULATION_DEATHRATECALCULATION_H
#define SIMS3000_POPULATION_DEATHRATECALCULATION_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"

namespace sims3000 {
namespace population {

namespace constants {
    constexpr float BASE_DEATH_RATE = 0.008f;           ///< 8 per 1000 per cycle
    constexpr float HEALTH_DEATH_MIN = 0.5f;            ///< Min health modifier (good health)
    constexpr float HEALTH_DEATH_MAX = 1.5f;            ///< Max health modifier (poor health)
    constexpr float CONTAMINATION_DEATH_MIN = 1.0f;     ///< Min contamination modifier (clean)
    constexpr float CONTAMINATION_DEATH_MAX = 2.0f;     ///< Max contamination modifier (polluted)
    constexpr float SERVICES_DEATH_MIN = 0.7f;          ///< Min services modifier (full coverage)
    constexpr float SERVICES_DEATH_MAX = 1.3f;          ///< Max services modifier (no coverage)
    constexpr float AGE_DEATH_MIN = 0.5f;               ///< Min age modifier (young population)
    constexpr float AGE_DEATH_MAX = 2.0f;               ///< Max age modifier (elderly population)
    constexpr float MAX_DEATH_PERCENT = 0.05f;          ///< 5% cap per cycle
} // namespace constants

/**
 * @struct DeathRateResult
 * @brief Result of death rate calculation.
 *
 * Contains the effective rate, individual modifiers, and actual death count.
 */
struct DeathRateResult {
    float effective_rate;              ///< Final rate per 1000
    float health_modifier;            ///< Health contribution to rate
    float contamination_modifier;     ///< Contamination contribution to rate
    float services_modifier;          ///< Service coverage contribution to rate
    float age_modifier;               ///< Age distribution contribution to rate
    uint32_t deaths;                  ///< Actual deaths this cycle
};

/**
 * @brief Calculate death rate for a population.
 *
 * Pure calculation function with no side effects. Computes deaths based on:
 * - Base death rate modified by health, contamination, services, and age
 * - Deaths capped at MAX_DEATH_PERCENT of total population per cycle
 * - Zero deaths if population is 0
 *
 * @param pop Population data for the city
 * @param contamination_level Environmental contamination level (0-100)
 * @param service_coverage Service coverage level (0-100)
 * @return DeathRateResult with effective rate, modifiers, and death count
 */
DeathRateResult calculate_death_rate(const PopulationData& pop,
                                     float contamination_level,
                                     float service_coverage);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_DEATHRATECALCULATION_H
