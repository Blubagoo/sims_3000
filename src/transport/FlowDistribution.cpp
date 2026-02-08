/**
 * @file FlowDistribution.cpp
 * @brief Flow distribution implementation (Epic 7, Ticket E7-013)
 *
 * Distributes building traffic flow to nearest pathway tiles.
 * Uses ProximityCache for fast distance filtering and BFS search
 * for finding the exact nearest pathway tile.
 *
 * @see FlowDistribution.h for class documentation.
 */

#include <sims3000/transport/FlowDistribution.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/ProximityCache.h>
#include <queue>

namespace sims3000 {
namespace transport {

// =============================================================================
// Position packing utilities
// =============================================================================

uint64_t FlowDistribution::pack_position(int32_t x, int32_t y) {
    // Store x in lower 32 bits, y in upper 32 bits
    uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
    uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
    return (uy << 32) | ux;
}

int32_t FlowDistribution::unpack_x(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFF));
}

int32_t FlowDistribution::unpack_y(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key >> 32));
}

// =============================================================================
// Flow distribution
// =============================================================================

uint32_t FlowDistribution::distribute_flow(
    const std::vector<BuildingTrafficSource>& sources,
    const PathwayGrid& grid,
    const ProximityCache& cache,
    std::unordered_map<uint64_t, uint32_t>& flow_accumulator,
    uint8_t max_distance)
{
    uint32_t connected_count = 0;

    for (const auto& source : sources) {
        // Step 1: Quick check via proximity cache
        uint8_t dist = cache.get_distance(source.x, source.y);
        if (dist > max_distance) {
            // Building is too far from any pathway - skip
            continue;
        }

        // Step 2: Find the exact nearest pathway tile
        int32_t px = 0, py = 0;
        if (!find_nearest_pathway(source.x, source.y, grid, px, py, max_distance)) {
            // Should not happen if proximity cache said we're within range,
            // but handle gracefully
            continue;
        }

        // Step 3: Add flow to accumulator (cross-ownership per CCR-002)
        uint64_t key = pack_position(px, py);
        flow_accumulator[key] += source.flow_amount;

        ++connected_count;
    }

    return connected_count;
}

// =============================================================================
// Internal helpers
// =============================================================================

bool FlowDistribution::find_nearest_pathway(
    int32_t x, int32_t y, const PathwayGrid& grid,
    int32_t& out_px, int32_t& out_py, uint8_t max_distance)
{
    // If the building is on a pathway tile, return it directly
    if (grid.has_pathway(x, y)) {
        out_px = x;
        out_py = y;
        return true;
    }

    // BFS search from building position to find nearest pathway
    // Uses Manhattan distance (4-directional: N/S/E/W)
    struct SearchNode {
        int32_t sx, sy;
        uint8_t dist;
    };

    std::queue<SearchNode> frontier;
    frontier.push({x, y, 0});

    // Track visited tiles to avoid revisiting
    // Use a simple set of packed positions
    std::unordered_map<uint64_t, bool> visited;
    visited[pack_position(x, y)] = true;

    static const int32_t dx[] = { 0, 0, 1, -1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    while (!frontier.empty()) {
        auto node = frontier.front();
        frontier.pop();

        if (node.dist >= max_distance) {
            continue;
        }

        uint8_t next_dist = node.dist + 1;

        for (int i = 0; i < 4; ++i) {
            int32_t nx = node.sx + dx[i];
            int32_t ny = node.sy + dy[i];

            uint64_t key = pack_position(nx, ny);
            if (visited.count(key) > 0) {
                continue;
            }
            visited[key] = true;

            if (!grid.in_bounds(nx, ny)) {
                continue;
            }

            if (grid.has_pathway(nx, ny)) {
                out_px = nx;
                out_py = ny;
                return true;
            }

            if (next_dist < max_distance) {
                frontier.push({nx, ny, next_dist});
            }
        }
    }

    return false;
}

} // namespace transport
} // namespace sims3000
