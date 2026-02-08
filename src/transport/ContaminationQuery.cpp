/**
 * @file ContaminationQuery.cpp
 * @brief Traffic contamination query implementation (Epic 7, Ticket E7-029)
 *
 * Implements position-based contamination queries using the PathwayGrid
 * and TrafficComponent data.
 *
 * @see ContaminationQuery.h for class documentation.
 */

#include <sims3000/transport/ContaminationQuery.h>

namespace sims3000 {
namespace transport {

uint8_t ContaminationQuery::get_contamination_rate_at(
    int32_t x, int32_t y,
    const PathwayGrid& grid,
    const std::unordered_map<uint32_t, TrafficComponent>& traffic_map)
{
    uint32_t entity_id = grid.get_pathway_at(x, y);
    if (entity_id == 0) {
        return 0;
    }

    auto it = traffic_map.find(entity_id);
    if (it == traffic_map.end()) {
        return 0;
    }

    return it->second.contamination_rate;
}

bool ContaminationQuery::has_traffic_contamination(
    int32_t x, int32_t y,
    const PathwayGrid& grid,
    const std::unordered_map<uint32_t, TrafficComponent>& traffic_map)
{
    uint32_t entity_id = grid.get_pathway_at(x, y);
    if (entity_id == 0) {
        return false;
    }

    auto it = traffic_map.find(entity_id);
    if (it == traffic_map.end()) {
        return false;
    }

    return it->second.congestion_level > 128;
}

} // namespace transport
} // namespace sims3000
