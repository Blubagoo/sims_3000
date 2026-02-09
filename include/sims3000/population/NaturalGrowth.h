/**
 * @file NaturalGrowth.h
 * @brief Natural population growth application (Ticket E10-017)
 *
 * Combines birth and death rate calculations to apply natural growth
 * to a PopulationData instance. Updates population totals and rates.
 */

#ifndef SIMS3000_POPULATION_NATURALGROWTH_H
#define SIMS3000_POPULATION_NATURALGROWTH_H

#include <cstdint>
#include "sims3000/population/PopulationData.h"

namespace sims3000 {
namespace population {

/**
 * @struct NaturalGrowthResult
 * @brief Result of applying natural growth to a population.
 *
 * Contains births, deaths, net natural growth, and updated total beings.
 */
struct NaturalGrowthResult {
    uint32_t births;            ///< Number of births this cycle
    uint32_t deaths;            ///< Number of deaths this cycle
    int32_t natural_growth;     ///< births - deaths
    uint32_t new_total_beings;  ///< Updated total population after growth
};

/**
 * @brief Apply natural growth to a population.
 *
 * Calls calculate_birth_rate() and calculate_death_rate() to determine
 * births and deaths, then updates the PopulationData in-place:
 * - pop.total_beings = max(0, total_beings + natural_growth)
 * - pop.natural_growth = births - deaths
 * - pop.birth_rate_per_1000 and pop.death_rate_per_1000 updated
 *
 * Population never goes below zero.
 *
 * @param pop Population data to update (modified in-place)
 * @param available_housing Number of available housing slots
 * @param contamination_level Environmental contamination level (0-100)
 * @param service_coverage Service coverage level (0-100)
 * @return NaturalGrowthResult with births, deaths, growth, and new total
 */
NaturalGrowthResult apply_natural_growth(PopulationData& pop,
                                          uint32_t available_housing,
                                          float contamination_level,
                                          float service_coverage);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_NATURALGROWTH_H
