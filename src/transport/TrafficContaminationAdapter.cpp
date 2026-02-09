/**
 * @file TrafficContaminationAdapter.cpp
 * @brief Implementation of TrafficContaminationAdapter (E10-115)
 */

#include <sims3000/transport/TrafficContaminationAdapter.h>
#include <algorithm>

namespace sims3000 {
namespace transport {

void TrafficContaminationAdapter::set_traffic_tiles(const std::vector<TrafficTileInfo>& tiles) {
    m_tiles = tiles;
}

void TrafficContaminationAdapter::clear() {
    m_tiles.clear();
}

void TrafficContaminationAdapter::get_contamination_sources(
    std::vector<contamination::ContaminationSourceEntry>& entries) const {

    for (const auto& tile : m_tiles) {
        // Skip inactive tiles
        if (!tile.is_active) {
            continue;
        }

        // Skip tiles below congestion threshold
        if (tile.congestion < MIN_CONGESTION_THRESHOLD) {
            continue;
        }

        // Clamp congestion to [0.0, 1.0]
        float clamped_congestion = std::max(0.0f, std::min(1.0f, tile.congestion));

        // Compute output: lerp(TRAFFIC_CONTAM_MIN, TRAFFIC_CONTAM_MAX, congestion)
        // lerp(a, b, t) = a + (b - a) * t
        float output_float = static_cast<float>(TRAFFIC_CONTAM_MIN) +
                            (static_cast<float>(TRAFFIC_CONTAM_MAX) - static_cast<float>(TRAFFIC_CONTAM_MIN)) * clamped_congestion;
        uint32_t output = static_cast<uint32_t>(output_float);

        // Add contamination source entry
        contamination::ContaminationSourceEntry entry;
        entry.x = tile.x;
        entry.y = tile.y;
        entry.output = output;
        entry.type = contamination::ContaminationType::Traffic;

        entries.push_back(entry);
    }
}

} // namespace transport
} // namespace sims3000
