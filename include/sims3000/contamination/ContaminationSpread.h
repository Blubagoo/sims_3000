/**
 * @file ContaminationSpread.h
 * @brief Contamination spread algorithm for grid-based diffusion.
 *
 * Implements the contamination spread system where contaminated tiles
 * spread contamination to their 8 neighbors using a delta buffer to
 * avoid order-dependent results.
 *
 * Spread algorithm:
 * - Only tiles with contamination >= CONTAM_SPREAD_THRESHOLD (32) spread
 * - Cardinal neighbors (4): receive level/8 contamination
 * - Diagonal neighbors (4): receive level/16 contamination
 * - Uses delta buffer to accumulate all spreads before applying
 * - Reads from previous tick buffer, writes to current buffer
 *
 * @see E10-087
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONSPREAD_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONSPREAD_H

#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdint>

namespace sims3000 {
namespace contamination {

/**
 * @brief Minimum contamination level required for a tile to spread.
 *
 * Tiles below this threshold do not contribute to spread.
 */
constexpr uint8_t CONTAM_SPREAD_THRESHOLD = 32;

/**
 * @brief Apply contamination spread across the entire grid.
 *
 * For each tile with contamination >= CONTAM_SPREAD_THRESHOLD, spreads
 * contamination to its 8 neighbors using the following formula:
 * - Cardinal neighbors (N, S, E, W): spread_amount = level / 8
 * - Diagonal neighbors (NE, NW, SE, SW): spread_amount = level / 16
 *
 * The spread algorithm:
 * 1. Reads contamination levels from the previous tick buffer
 * 2. Accumulates spread amounts in a delta buffer
 * 3. Applies all deltas to the current buffer in a single pass
 *
 * This ensures spread results are independent of iteration order.
 *
 * @param grid The contamination grid to update.
 */
void apply_contamination_spread(ContaminationGrid& grid);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONSPREAD_H
