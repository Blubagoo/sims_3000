/**
 * @file FlowPropagation.cpp
 * @brief Implementation of FlowPropagation diffusion model (Epic 7, Ticket E7-014)
 *
 * Single-pass diffusion: for each pathway tile with flow, a fraction
 * (spread_rate) spreads equally to connected pathway neighbors.
 *
 * @see FlowPropagation.h for class documentation.
 */

#include <sims3000/transport/FlowPropagation.h>
#include <sims3000/transport/PathwayGrid.h>

namespace sims3000 {
namespace transport {

// =============================================================================
// Position packing (same convention as FlowDistribution)
// =============================================================================

static uint64_t pack_pos(int32_t x, int32_t y) {
    uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
    uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
    return (uy << 32) | ux;
}

static int32_t unpack_x(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFF));
}

static int32_t unpack_y(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key >> 32));
}

// =============================================================================
// Flow propagation
// =============================================================================

void FlowPropagation::propagate(
    std::unordered_map<uint64_t, uint32_t>& flow_map,
    const PathwayGrid& grid,
    const FlowPropagationConfig& config)
{
    if (flow_map.empty()) {
        return;
    }

    // Step 1: Snapshot current flow values to avoid read-after-write issues
    // Also pre-compute neighbor lists for each tile with flow
    struct SpreadInfo {
        uint64_t source_key;
        uint32_t spread_amount;
        std::vector<uint64_t> neighbors;
    };

    std::vector<SpreadInfo> spreads;
    spreads.reserve(flow_map.size());

    for (const auto& entry : flow_map) {
        uint32_t current_flow = entry.second;
        if (current_flow == 0) {
            continue;
        }

        int32_t x = unpack_x(entry.first);
        int32_t y = unpack_y(entry.first);

        // Get connected pathway neighbors
        auto neighbors = get_pathway_neighbors(x, y, grid);
        if (neighbors.empty()) {
            continue;
        }

        // Compute total spread amount
        uint32_t spread_total = static_cast<uint32_t>(
            static_cast<float>(current_flow) * config.spread_rate
        );

        if (spread_total == 0) {
            continue;
        }

        SpreadInfo info;
        info.source_key = entry.first;
        info.spread_amount = spread_total;
        info.neighbors = std::move(neighbors);
        spreads.push_back(std::move(info));
    }

    // Step 2: Apply spread operations
    for (const auto& info : spreads) {
        uint32_t neighbor_count = static_cast<uint32_t>(info.neighbors.size());
        uint32_t per_neighbor = info.spread_amount / neighbor_count;

        if (per_neighbor == 0) {
            continue;
        }

        // Actual amount removed from source = per_neighbor * neighbor_count
        // (integer division may lose some flow, which is acceptable)
        uint32_t actual_spread = per_neighbor * neighbor_count;

        // Subtract from source
        if (flow_map[info.source_key] >= actual_spread) {
            flow_map[info.source_key] -= actual_spread;
        } else {
            flow_map[info.source_key] = 0;
        }

        // Add to each neighbor
        for (uint64_t neighbor_key : info.neighbors) {
            flow_map[neighbor_key] += per_neighbor;
        }
    }
}

// =============================================================================
// Neighbor lookup
// =============================================================================

std::vector<uint64_t> FlowPropagation::get_pathway_neighbors(
    int32_t x, int32_t y, const PathwayGrid& grid)
{
    std::vector<uint64_t> neighbors;
    neighbors.reserve(4);

    static const int32_t dx[] = { 0, 0, 1, -1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    for (int i = 0; i < 4; ++i) {
        int32_t nx = x + dx[i];
        int32_t ny = y + dy[i];

        if (grid.has_pathway(nx, ny)) {
            neighbors.push_back(pack_pos(nx, ny));
        }
    }

    return neighbors;
}

} // namespace transport
} // namespace sims3000
