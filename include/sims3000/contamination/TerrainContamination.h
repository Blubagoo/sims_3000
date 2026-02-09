/**
 * @file TerrainContamination.h
 * @brief Terrain-based contamination from blight mires (Ticket E10-086).
 *
 * Certain terrain tiles (blight mires, toxic swamps) emit a fixed amount
 * of contamination each tick. This module applies that contamination to
 * the ContaminationGrid.
 *
 * @see ContaminationGrid
 * @see ContaminationType::Terrain
 * @see E10-086
 */

#ifndef SIMS3000_CONTAMINATION_TERRAINCONTAMINATION_H
#define SIMS3000_CONTAMINATION_TERRAINCONTAMINATION_H

#include <cstdint>
#include <vector>

#include "sims3000/contamination/ContaminationGrid.h"

namespace sims3000 {
namespace contamination {

/// Contamination amount emitted per tick by each blight mire tile
constexpr uint8_t BLIGHT_MIRE_CONTAMINATION = 30;

/**
 * @struct TerrainContaminationSource
 * @brief Grid position of a toxic terrain tile.
 */
struct TerrainContaminationSource {
    int32_t x; ///< Grid X coordinate
    int32_t y; ///< Grid Y coordinate
};

/**
 * @brief Apply terrain-based contamination to the grid.
 *
 * For each toxic tile in the list, adds BLIGHT_MIRE_CONTAMINATION (30)
 * to the grid at that position with ContaminationType::Terrain.
 *
 * @param grid The contamination grid to write to.
 * @param toxic_tiles List of toxic terrain tile positions.
 */
void apply_terrain_contamination(ContaminationGrid& grid,
                                  const std::vector<TerrainContaminationSource>& toxic_tiles);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_TERRAINCONTAMINATION_H
