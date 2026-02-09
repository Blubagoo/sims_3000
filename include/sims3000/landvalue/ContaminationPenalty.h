/**
 * @file ContaminationPenalty.h
 * @brief Contamination penalty calculation for land value system.
 *
 * Implements the penalty applied to land value based on contamination levels.
 * Higher contamination reduces land value, making areas less desirable.
 *
 * Penalty calculation:
 * - Contamination range: 0-255
 * - Penalty range: 0-50
 * - Formula: penalty = (contamination * 50) / 255
 * - Reads from previous tick's contamination buffer
 *
 * @see E10-104
 */

#ifndef SIMS3000_LANDVALUE_CONTAMINATIONPENALTY_H
#define SIMS3000_LANDVALUE_CONTAMINATIONPENALTY_H

#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdint>

namespace sims3000 {
namespace landvalue {

/**
 * @brief Maximum penalty that can be applied for contamination.
 *
 * This is the penalty applied when contamination is at maximum (255).
 */
constexpr uint8_t MAX_CONTAMINATION_PENALTY = 50;

/**
 * @brief Calculate contamination penalty for a single tile.
 *
 * Scales contamination level (0-255) to penalty amount (0-50) using
 * the formula: penalty = (contamination * 50) / 255
 *
 * @param contamination_level Contamination level 0-255.
 * @return Penalty amount 0-50.
 */
uint8_t calculate_contamination_penalty(uint8_t contamination_level);

/**
 * @brief Apply contamination penalties across entire grid.
 *
 * For each tile in the grid:
 * 1. Read contamination level from previous tick's contamination grid
 * 2. Calculate penalty amount
 * 3. Subtract penalty from land value grid
 *
 * Uses saturating subtraction so land value cannot go below 0.
 *
 * @param value_grid Land value grid to update.
 * @param contam_grid Contamination grid to read from (previous tick buffer).
 */
void apply_contamination_penalties(LandValueGrid& value_grid, const contamination::ContaminationGrid& contam_grid);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_CONTAMINATIONPENALTY_H
