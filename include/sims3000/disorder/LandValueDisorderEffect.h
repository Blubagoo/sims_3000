/**
 * @file LandValueDisorderEffect.h
 * @brief Land value modifier applied to existing disorder levels (Ticket E10-074).
 *
 * Low land value areas amplify existing disorder. High land value areas
 * do not add additional disorder. The effect is proportional:
 * - Land value 0: +100% disorder (doubles existing)
 * - Land value 255: +0% additional disorder
 *
 * @see DisorderGrid
 * @see LandValueGrid
 * @see E10-074
 */

#ifndef SIMS3000_DISORDER_LANDVALUEDISORDEREFFECT_H
#define SIMS3000_DISORDER_LANDVALUEDISORDEREFFECT_H

#include "sims3000/disorder/DisorderGrid.h"
#include "sims3000/landvalue/LandValueGrid.h"

namespace sims3000 {
namespace disorder {

/**
 * @brief Apply land value modifier to existing disorder in the grid.
 *
 * For each cell where disorder > 0:
 *   extra = disorder * (1.0 - land_value / 255.0)
 *   new_disorder = disorder + extra (saturating at 255)
 *
 * Low land value (0) = +100% disorder generation boost (doubles existing).
 * High land value (255) = +0% additional disorder.
 *
 * @param grid The disorder grid to modify (reads and writes current buffer).
 * @param land_value_grid The land value grid to read from.
 */
void apply_land_value_effect(DisorderGrid& grid, const landvalue::LandValueGrid& land_value_grid);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_LANDVALUEDISORDEREFFECT_H
