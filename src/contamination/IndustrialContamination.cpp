/**
 * @file IndustrialContamination.cpp
 * @brief Implementation of industrial contamination generation.
 *
 * @see IndustrialContamination.h for function documentation.
 * @see E10-083
 */

#include <sims3000/contamination/IndustrialContamination.h>
#include <sims3000/contamination/ContaminationType.h>
#include <algorithm>

namespace sims3000 {
namespace contamination {

void apply_industrial_contamination(ContaminationGrid& grid,
                                     const std::vector<IndustrialSource>& sources) {
    for (const auto& src : sources) {
        if (!src.is_active) {
            continue;
        }

        // building_level is 1-3, index into INDUSTRIAL_BASE_OUTPUT is 0-2
        if (src.building_level < 1 || src.building_level > 3) {
            continue;
        }

        uint8_t base = INDUSTRIAL_BASE_OUTPUT[src.building_level - 1];
        float output_f = static_cast<float>(base) * src.occupancy_ratio;
        uint8_t output = static_cast<uint8_t>(std::min(output_f, 255.0f));

        grid.add_contamination(src.x, src.y, output,
                               static_cast<uint8_t>(ContaminationType::Industrial));
    }
}

} // namespace contamination
} // namespace sims3000
