/**
 * @file DisorderPenalty.h
 * @brief Disorder penalty calculation for land value system.
 *
 * Implements the penalty applied to land value based on disorder levels.
 * Higher disorder reduces land value, making areas less desirable.
 *
 * Penalty calculation:
 * - Disorder range: 0-255
 * - Penalty range: 0-40
 * - Formula: penalty = (disorder * 40) / 255
 * - Reads from previous tick's disorder buffer
 *
 * @see E10-103
 */

#ifndef SIMS3000_LANDVALUE_DISORDERPENALTY_H
#define SIMS3000_LANDVALUE_DISORDERPENALTY_H

#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdint>

namespace sims3000 {
namespace landvalue {

/**
 * @brief Maximum penalty that can be applied for disorder.
 *
 * This is the penalty applied when disorder is at maximum (255).
 */
constexpr uint8_t MAX_DISORDER_PENALTY = 40;

/**
 * @brief Calculate disorder penalty for a single tile.
 *
 * Scales disorder level (0-255) to penalty amount (0-40) using
 * the formula: penalty = (disorder * 40) / 255
 *
 * @param disorder_level Disorder level 0-255.
 * @return Penalty amount 0-40.
 */
uint8_t calculate_disorder_penalty(uint8_t disorder_level);

/**
 * @brief Apply disorder penalties across entire grid.
 *
 * For each tile in the grid:
 * 1. Read disorder level from previous tick's disorder grid
 * 2. Calculate penalty amount
 * 3. Subtract penalty from land value grid
 *
 * Uses saturating subtraction so land value cannot go below 0.
 *
 * @param value_grid Land value grid to update.
 * @param disorder_grid Disorder grid to read from (previous tick buffer).
 */
void apply_disorder_penalties(LandValueGrid& value_grid, const disorder::DisorderGrid& disorder_grid);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_DISORDERPENALTY_H
