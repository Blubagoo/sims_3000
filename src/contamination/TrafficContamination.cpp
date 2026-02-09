/**
 * @file TrafficContamination.cpp
 * @brief Implementation of traffic contamination generation.
 *
 * @see TrafficContamination.h for function documentation.
 * @see E10-085
 */

#include <sims3000/contamination/TrafficContamination.h>
#include <sims3000/contamination/ContaminationType.h>
#include <algorithm>

namespace sims3000 {
namespace contamination {

void apply_traffic_contamination(ContaminationGrid& grid,
                                  const std::vector<TrafficSource>& sources) {
    for (const auto& src : sources) {
        // Clamp congestion to [0, 1]
        float congestion = std::max(0.0f, std::min(1.0f, src.congestion));

        // Linear interpolation: output = MIN + (MAX - MIN) * congestion
        float output_f = static_cast<float>(TRAFFIC_CONTAMINATION_MIN) +
                         static_cast<float>(TRAFFIC_CONTAMINATION_MAX - TRAFFIC_CONTAMINATION_MIN) * congestion;
        uint8_t output = static_cast<uint8_t>(output_f);

        grid.add_contamination(src.x, src.y, output,
                               static_cast<uint8_t>(ContaminationType::Traffic));
    }
}

} // namespace contamination
} // namespace sims3000
