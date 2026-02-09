/**
 * @file PopulationData.h
 * @brief Per-player aggregate population data (Ticket E10-010)
 *
 * This is the per-PLAYER aggregate population component, distinct from
 * the per-building PopulationComponent in ecs/Components.h.
 *
 * Tracks total city population, age distribution, birth/death rates,
 * migration, growth metrics, and quality-of-life indices.
 */

#ifndef SIMS3000_POPULATION_POPULATIONDATA_H
#define SIMS3000_POPULATION_POPULATIONDATA_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @struct PopulationData
 * @brief Per-player aggregate population state.
 *
 * Contains all population metrics for a single player's city:
 * - Total beings and capacity
 * - Age distribution (youth/adult/elder percentages, sum to 100)
 * - Birth and death rates per 1000 per cycle
 * - Derived growth/migration values
 * - Quality-of-life indices (0-100 scale)
 * - Historical ring buffer (12 entries)
 *
 * Target size: ~90 bytes.
 */
struct PopulationData {
    /// Total number of beings in the city
    uint32_t total_beings = 0;

    /// Maximum housing capacity across all residential buildings
    uint32_t max_capacity = 0;

    /// Age distribution percentages (must sum to 100)
    uint8_t youth_percent = 33;   ///< Percentage of youth population
    uint8_t adult_percent = 34;   ///< Percentage of adult (working age) population
    uint8_t elder_percent = 33;   ///< Percentage of elder population

    /// Demographic rates per 1000 beings per simulation cycle
    uint16_t birth_rate_per_1000 = 15;  ///< Birth rate per 1000 per cycle
    uint16_t death_rate_per_1000 = 8;   ///< Death rate per 1000 per cycle

    /// Derived growth metrics (computed by PopulationSystem)
    int32_t natural_growth = 0;   ///< Births minus deaths this cycle
    int32_t net_migration = 0;    ///< Net migration (positive = inflow)
    float growth_rate = 0.0f;     ///< Overall growth rate as fraction

    /// Quality-of-life indices (0-100 scale, 50 = neutral)
    uint8_t harmony_index = 50;    ///< Social harmony / happiness
    uint8_t health_index = 50;     ///< Public health quality
    uint8_t education_index = 50;  ///< Education quality

    /// Historical ring buffer for population tracking (12 entries)
    uint32_t population_history[12] = {};  ///< Past population snapshots
    uint8_t history_index = 0;             ///< Current write position in ring buffer
};

static_assert(sizeof(PopulationData) <= 96, "PopulationData should be approximately 90 bytes");

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_POPULATIONDATA_H
