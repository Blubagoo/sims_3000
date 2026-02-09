/**
 * @file DisorderGeneration.h
 * @brief Disorder generation from buildings based on zone type, occupancy,
 *        and land value (Ticket E10-073).
 *
 * Provides configuration and calculation for per-building disorder generation.
 * Each zone type has a base generation rate, a population multiplier (scaled
 * by occupancy), and a land value modifier (scaled by inverse land value).
 *
 * @see DisorderGrid
 * @see E10-073
 */

#ifndef SIMS3000_DISORDER_DISORDERGENERATION_H
#define SIMS3000_DISORDER_DISORDERGENERATION_H

#include <cstdint>

#include "sims3000/disorder/DisorderGrid.h"

namespace sims3000 {
namespace disorder {

/**
 * @struct DisorderGenerationConfig
 * @brief Configuration for disorder generation per zone type.
 *
 * Defines how much disorder a building of a given zone type generates
 * based on its base rate, occupancy, and surrounding land value.
 */
struct DisorderGenerationConfig {
    uint8_t base_generation;       ///< Base disorder output per tick
    float population_multiplier;   ///< Multiplier scaled by occupancy ratio (0-1)
    float land_value_modifier;     ///< Multiplier scaled by inverse land value (0-1)
};

/**
 * @brief Default disorder generation configs indexed by zone type.
 *
 * Zone type indices:
 * - 0: hab_low (low-density habitation)
 * - 1: hab_high (high-density habitation)
 * - 2: exchange_low (low-density commercial/exchange)
 * - 3: exchange_high (high-density commercial/exchange)
 * - 4: fab (fabrication/industrial)
 */
constexpr DisorderGenerationConfig DISORDER_CONFIGS[] = {
    { 2, 0.5f, 0.3f },  ///< hab_low: 2 base
    { 5, 0.8f, 0.5f },  ///< hab_high: 5 base
    { 3, 0.4f, 0.2f },  ///< exchange_low: 3 base
    { 6, 0.6f, 0.3f },  ///< exchange_high: 6 base
    { 1, 0.2f, 0.1f },  ///< fab: 1 base
};

/// Number of zone types with disorder configs
constexpr uint8_t DISORDER_CONFIG_COUNT = 5;

/**
 * @struct DisorderSource
 * @brief Represents a single building that generates disorder.
 *
 * Contains the building's grid position, zone type, occupancy ratio,
 * and local land value -- all factors that influence disorder output.
 */
struct DisorderSource {
    int32_t x;              ///< Grid X coordinate
    int32_t y;              ///< Grid Y coordinate
    uint8_t zone_type;      ///< Index into DISORDER_CONFIGS (0-4)
    float occupancy_ratio;  ///< Occupancy ratio 0.0-1.0
    uint8_t land_value;     ///< Local land value 0-255
};

/**
 * @brief Calculate the disorder generation amount for a single source.
 *
 * Formula:
 * - generation = base + (base * population_multiplier * occupancy_ratio)
 * - land_value_mod = land_value_modifier * (1.0 - land_value / 255.0)
 * - generation += generation * land_value_mod
 * - result clamped to [0, 255]
 *
 * @param source The disorder source to calculate for.
 * @return Disorder amount 0-255.
 */
uint8_t calculate_disorder_amount(const DisorderSource& source);

/**
 * @brief Calculate disorder for a source and apply it to the grid.
 *
 * Calls calculate_disorder_amount() and then grid.add_disorder()
 * at the source's position.
 *
 * @param grid The disorder grid to write to.
 * @param source The disorder source.
 */
void apply_disorder_generation(DisorderGrid& grid, const DisorderSource& source);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERGENERATION_H
