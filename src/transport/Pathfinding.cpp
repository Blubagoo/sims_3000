/**
 * @file Pathfinding.cpp
 * @brief A* pathfinding implementation (Epic 7, Ticket E7-023)
 *
 * Standard A* with Manhattan distance heuristic.
 * Early exit via network_id check (different connected component = no path).
 * Base edge cost = 10 (enhanced by E7-024).
 *
 * @see Pathfinding.h for class documentation.
 */

#include <sims3000/transport/Pathfinding.h>
#include <sims3000/transport/PathwayGrid.h>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace transport {

// =============================================================================
// Position packing for hash map keys
// =============================================================================

static uint64_t pack_pos(int32_t x, int32_t y) {
    uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
    uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
    return (uy << 32) | ux;
}

// =============================================================================
// A* implementation
// =============================================================================

PathResult Pathfinding::find_path(
    const GridPosition& start,
    const GridPosition& end,
    const PathwayGrid& grid,
    const NetworkGraph& graph)
{
    PathResult result;

    // Validate start is a pathway tile
    if (!grid.has_pathway(start.x, start.y)) {
        return result;
    }

    // Validate end is a pathway tile
    if (!grid.has_pathway(end.x, end.y)) {
        return result;
    }

    // Trivial case: start == end
    if (start == end) {
        result.found = true;
        result.path.push_back(start);
        result.total_cost = 0;
        return result;
    }

    // Early exit: check if start and end are in the same connected component
    uint16_t start_net = graph.get_network_id(start);
    uint16_t end_net = graph.get_network_id(end);

    if (start_net == 0 || end_net == 0 || start_net != end_net) {
        // Different networks or not in graph -> no path possible
        return result;
    }

    // A* data structures
    struct AStarNode {
        GridPosition pos;
        uint32_t g_cost;     // cost from start
        uint32_t f_cost;     // g_cost + heuristic
        bool operator>(const AStarNode& other) const { return f_cost > other.f_cost; }
    };

    // Open set: min-heap by f_cost
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;

    // g_cost for each visited position
    std::unordered_map<uint64_t, uint32_t> g_costs;

    // Parent tracking for path reconstruction
    std::unordered_map<uint64_t, uint64_t> came_from;

    // Closed set
    std::unordered_map<uint64_t, bool> closed;

    // Initialize start node
    uint64_t start_key = pack_pos(start.x, start.y);
    uint64_t end_key = pack_pos(end.x, end.y);

    uint32_t h = heuristic(start, end);
    open_set.push({start, 0, h});
    g_costs[start_key] = 0;

    // Cardinal directions: N, S, E, W
    static const int32_t dx[] = { 0, 0, 1, -1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    while (!open_set.empty()) {
        AStarNode current = open_set.top();
        open_set.pop();

        uint64_t current_key = pack_pos(current.pos.x, current.pos.y);

        // Skip if already processed
        if (closed.count(current_key) > 0) {
            continue;
        }
        closed[current_key] = true;

        // Check if we reached the goal
        if (current_key == end_key) {
            // Reconstruct path
            result.found = true;
            result.total_cost = current.g_cost;

            // Build path by following came_from chain
            std::vector<GridPosition> reversed_path;
            uint64_t trace = current_key;

            while (true) {
                int32_t tx = static_cast<int32_t>(static_cast<uint32_t>(trace & 0xFFFFFFFF));
                int32_t ty = static_cast<int32_t>(static_cast<uint32_t>(trace >> 32));
                reversed_path.push_back({tx, ty});

                auto it = came_from.find(trace);
                if (it == came_from.end()) {
                    break;
                }
                trace = it->second;
            }

            // Reverse to get start-to-end order
            result.path.reserve(reversed_path.size());
            for (auto it = reversed_path.rbegin(); it != reversed_path.rend(); ++it) {
                result.path.push_back(*it);
            }

            return result;
        }

        // Explore neighbors
        for (int i = 0; i < 4; ++i) {
            int32_t nx = current.pos.x + dx[i];
            int32_t ny = current.pos.y + dy[i];

            // Must be a pathway tile
            if (!grid.has_pathway(nx, ny)) {
                continue;
            }

            uint64_t neighbor_key = pack_pos(nx, ny);

            // Skip if already in closed set
            if (closed.count(neighbor_key) > 0) {
                continue;
            }

            GridPosition neighbor_pos{nx, ny};
            uint32_t tentative_g = current.g_cost + edge_cost(current.pos, neighbor_pos);

            // Check if this is a better path
            auto it = g_costs.find(neighbor_key);
            if (it != g_costs.end() && tentative_g >= it->second) {
                continue;
            }

            // Update best path
            g_costs[neighbor_key] = tentative_g;
            came_from[neighbor_key] = current_key;

            uint32_t f = tentative_g + heuristic(neighbor_pos, end);
            open_set.push({neighbor_pos, tentative_g, f});
        }
    }

    // No path found
    return result;
}

// =============================================================================
// Heuristic and cost
// =============================================================================

uint32_t Pathfinding::heuristic(const GridPosition& a, const GridPosition& b) {
    // Manhattan distance * base edge cost (10)
    uint32_t dx = static_cast<uint32_t>(std::abs(a.x - b.x));
    uint32_t dy = static_cast<uint32_t>(std::abs(a.y - b.y));
    return (dx + dy) * 10;
}

uint32_t Pathfinding::edge_cost(const GridPosition& /*from*/, const GridPosition& /*to*/) {
    // Base cost = 10 per step. E7-024 will enhance this.
    return 10;
}

} // namespace transport
} // namespace sims3000
