/**
 * @file TerrainContamination.cpp
 * @brief Implementation of terrain-based contamination (Ticket E10-086).
 *
 * @see TerrainContamination.h for interface documentation.
 * @see E10-086
 */

#include <sims3000/contamination/TerrainContamination.h>
#include <sims3000/contamination/ContaminationType.h>

namespace sims3000 {
namespace contamination {

void apply_terrain_contamination(ContaminationGrid& grid,
                                  const std::vector<TerrainContaminationSource>& toxic_tiles) {
    for (const TerrainContaminationSource& tile : toxic_tiles) {
        grid.add_contamination(tile.x, tile.y, BLIGHT_MIRE_CONTAMINATION,
                               static_cast<uint8_t>(ContaminationType::Terrain));
    }
}

} // namespace contamination
} // namespace sims3000
