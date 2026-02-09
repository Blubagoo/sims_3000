/**
 * @file DisorderSpread.h
 * @brief Disorder spread algorithm using delta buffer (Ticket E10-075).
 *
 * Spreads disorder to 4-neighbors when a cell exceeds the spread threshold.
 * Uses a delta buffer to avoid order-dependent results. Water tiles block
 * spread propagation.
 *
 * @see DisorderGrid
 * @see E10-075
 */

#ifndef SIMS3000_DISORDER_DISORDERSPREAD_H
#define SIMS3000_DISORDER_DISORDERSPREAD_H

#include <cstdint>
#include <vector>

#include "sims3000/disorder/DisorderGrid.h"

namespace sims3000 {
namespace disorder {

/// Minimum disorder level required for spreading to neighbors
constexpr uint8_t SPREAD_THRESHOLD = 64;

/**
 * @brief Spread disorder to 4-neighbors using a delta buffer.
 *
 * Algorithm:
 * 1. Create delta buffer (same size as grid, all zeros)
 * 2. For each cell where get_level(x,y) > SPREAD_THRESHOLD:
 *    - spread = (level - SPREAD_THRESHOLD) / 8
 *    - For each valid 4-neighbor that is not water:
 *      - delta[neighbor] += spread
 *    - delta[source] -= spread * num_valid_non_water_neighbors
 * 3. Apply deltas: set_level(x,y, clamp(level + delta, 0, 255))
 *
 * @param grid The disorder grid to spread (reads and writes current buffer).
 * @param water_mask Optional water mask. If provided, water_mask[y*width+x]
 *        indicates water tiles that block spread.
 */
void apply_disorder_spread(DisorderGrid& grid, const std::vector<bool>* water_mask = nullptr);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERSPREAD_H
