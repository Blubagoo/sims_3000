/**
 * @file ConnectivityQuery.cpp
 * @brief Implementation of O(1) connectivity query utility (Epic 7, Ticket E7-011)
 *
 * @see ConnectivityQuery.h
 */

#include <sims3000/transport/ConnectivityQuery.h>

namespace sims3000 {
namespace transport {

bool ConnectivityQuery::is_connected(const PathwayGrid& grid, const NetworkGraph& graph,
                                      int32_t x1, int32_t y1, int32_t x2, int32_t y2) const {
    // Check if both positions have pathways
    if (!grid.has_pathway(x1, y1) || !grid.has_pathway(x2, y2)) {
        return false;
    }

    // Look up network_ids via NetworkGraph
    uint16_t id1 = graph.get_network_id(GridPosition{x1, y1});
    uint16_t id2 = graph.get_network_id(GridPosition{x2, y2});

    // Both must have non-zero network_id and they must match
    return id1 != 0 && id1 == id2;
}

bool ConnectivityQuery::is_on_network(const PathwayGrid& grid, const NetworkGraph& graph,
                                       int32_t x, int32_t y) const {
    if (!grid.has_pathway(x, y)) {
        return false;
    }

    uint16_t id = graph.get_network_id(GridPosition{x, y});
    return id != 0;
}

uint16_t ConnectivityQuery::get_network_id_at(const PathwayGrid& grid, const NetworkGraph& graph,
                                               int32_t x, int32_t y) const {
    if (!grid.has_pathway(x, y)) {
        return 0;
    }

    return graph.get_network_id(GridPosition{x, y});
}

} // namespace transport
} // namespace sims3000
