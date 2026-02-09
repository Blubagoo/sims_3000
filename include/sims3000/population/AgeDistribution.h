/**
 * @file AgeDistribution.h
 * @brief Age distribution evolution over time (Ticket E10-018)
 *
 * Evolves age distribution (youth/adult/elder percentages) based on
 * births, deaths, and aging transitions between demographic cycles.
 */

#ifndef SIMS3000_POPULATION_AGEDISTRIBUTION_H
#define SIMS3000_POPULATION_AGEDISTRIBUTION_H

#include <cstdint>

namespace sims3000 {
namespace population {

/// Rate at which youth age into adults per demographic cycle (2%)
constexpr float YOUTH_TO_ADULT_RATE = 0.02f;

/// Rate at which adults age into elders per demographic cycle (1%)
constexpr float ADULT_TO_ELDER_RATE = 0.01f;

/// Proportion of deaths that come from elder population (60%)
constexpr float ELDER_DEATH_WEIGHT = 0.6f;

/// Proportion of deaths that come from adult population (30%)
constexpr float ADULT_DEATH_WEIGHT = 0.3f;

/// Proportion of deaths that come from youth population (10%)
constexpr float YOUTH_DEATH_WEIGHT = 0.1f;

/**
 * @struct AgeDistributionResult
 * @brief Result of age distribution evolution.
 *
 * Contains the new age distribution percentages after applying
 * births, deaths, and aging transitions. Percentages always sum to 100.
 */
struct AgeDistributionResult {
    uint8_t youth_percent;   ///< New percentage of youth (0-100)
    uint8_t adult_percent;   ///< New percentage of adults (0-100)
    uint8_t elder_percent;   ///< New percentage of elders (0-100)
};

/**
 * @brief Evolve age distribution for one demographic cycle.
 *
 * Algorithm:
 * 1. Convert percentages to counts
 * 2. Apply aging transitions (youth->adult, adult->elder)
 * 3. Apply births (add to youth)
 * 4. Apply weighted deaths (60% elder, 30% adult, 10% youth), never negative
 * 5. Recompute percentages ensuring they sum to 100
 *
 * @param current_youth Current youth percentage (0-100)
 * @param current_adult Current adult percentage (0-100)
 * @param current_elder Current elder percentage (0-100)
 * @param births Number of births this cycle
 * @param deaths Number of deaths this cycle
 * @param total_beings Total population count
 * @return New age distribution with percentages summing to 100
 */
AgeDistributionResult evolve_age_distribution(
    uint8_t current_youth,
    uint8_t current_adult,
    uint8_t current_elder,
    uint32_t births,
    uint32_t deaths,
    uint32_t total_beings
);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_AGEDISTRIBUTION_H
