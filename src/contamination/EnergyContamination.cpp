/**
 * @file EnergyContamination.cpp
 * @brief Implementation of energy contamination generation.
 *
 * @see EnergyContamination.h for function documentation.
 * @see E10-084
 */

#include <sims3000/contamination/EnergyContamination.h>
#include <sims3000/contamination/ContaminationType.h>

namespace sims3000 {
namespace contamination {

void apply_energy_contamination(ContaminationGrid& grid,
                                 const std::vector<EnergySource>& sources) {
    for (const auto& src : sources) {
        if (!src.is_active) {
            continue;
        }

        // Clean energy (type >= 3) produces 0 contamination
        if (src.nexus_type >= 3) {
            continue;
        }

        uint8_t output = ENERGY_CONTAMINATION_OUTPUT[src.nexus_type];

        grid.add_contamination(src.x, src.y, output,
                               static_cast<uint8_t>(ContaminationType::Energy));
    }
}

} // namespace contamination
} // namespace sims3000
