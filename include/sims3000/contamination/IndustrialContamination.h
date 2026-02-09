/**
 * @file IndustrialContamination.h
 * @brief Industrial contamination generation from fabrication buildings (Ticket E10-083).
 *
 * Fabrication buildings produce industrial contamination based on their
 * building level (density) and occupancy ratio.
 *
 * @see ContaminationGrid
 * @see ContaminationType
 * @see E10-083
 */

#ifndef SIMS3000_CONTAMINATION_INDUSTRIALCONTAMINATION_H
#define SIMS3000_CONTAMINATION_INDUSTRIALCONTAMINATION_H

#include <cstdint>
#include <vector>

#include "sims3000/contamination/ContaminationGrid.h"

namespace sims3000 {
namespace contamination {

/**
 * @struct IndustrialSource
 * @brief Represents a fabrication building that generates industrial contamination.
 */
struct IndustrialSource {
    int32_t x, y;              ///< Grid position
    uint8_t building_level;    ///< 1-3 (low/medium/high density)
    float occupancy_ratio;     ///< 0-1 occupancy fraction
    bool is_active;            ///< Whether the building is currently operational
};

/// Base contamination output per building level (index 0=level 1, 1=level 2, 2=level 3)
constexpr uint8_t INDUSTRIAL_BASE_OUTPUT[] = { 50, 100, 200 };

/**
 * @brief Apply industrial contamination from fabrication buildings to the grid.
 *
 * Per source: if is_active, output = INDUSTRIAL_BASE_OUTPUT[level-1] * occupancy_ratio.
 * Calls add_contamination(x, y, min(output, 255), ContaminationType::Industrial).
 * Inactive sources produce 0 contamination.
 *
 * @param grid The contamination grid to write to.
 * @param sources Vector of industrial contamination sources.
 */
void apply_industrial_contamination(ContaminationGrid& grid,
                                     const std::vector<IndustrialSource>& sources);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_INDUSTRIALCONTAMINATION_H
