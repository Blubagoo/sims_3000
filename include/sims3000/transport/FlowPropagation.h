/**
 * @file FlowPropagation.h
 * @brief Flow propagation diffusion model for transport network (Epic 7, Ticket E7-014)
 *
 * Propagates traffic flow across connected pathway tiles using a simple
 * diffusion model. Each tick, a fraction of flow at each tile spreads
 * equally to connected neighbors.
 *
 * No per-vehicle simulation (CCR-006). Flow is aggregate only.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sims3000 {
namespace transport {

class PathwayGrid;  // forward declaration

/**
 * @struct FlowPropagationConfig
 * @brief Configuration for flow propagation diffusion.
 */
struct FlowPropagationConfig {
    float spread_rate = 0.20f;  ///< Fraction of flow that spreads to neighbors per tick (20%)
};

/**
 * @class FlowPropagation
 * @brief Propagates traffic flow across connected pathways via diffusion.
 *
 * Single-pass per tick: for each pathway tile with flow, spread_rate of
 * the flow spreads equally to connected neighbors. Distribution is
 * proportional to neighbor count (equal split for simplicity).
 *
 * This is an aggregate model - no per-vehicle simulation (CCR-006).
 */
class FlowPropagation {
public:
    FlowPropagation() = default;

    /**
     * @brief Propagate flow across connected pathways.
     *
     * Algorithm:
     * 1. Snapshot current flow values
     * 2. For each tile with flow, compute spread amount = flow * spread_rate
     * 3. Divide spread equally among connected pathway neighbors
     * 4. Subtract spread from source tile, add shares to neighbors
     *
     * @param flow_map   Position (packed uint64_t) -> current flow amount.
     *                   Updated in-place with diffused values.
     * @param grid       PathwayGrid for connectivity lookup.
     * @param config     Propagation configuration (spread rate).
     */
    void propagate(
        std::unordered_map<uint64_t, uint32_t>& flow_map,
        const PathwayGrid& grid,
        const FlowPropagationConfig& config = FlowPropagationConfig{}
    );

private:
    /**
     * @brief Get connected neighbor positions with pathways (N/S/E/W).
     *
     * Returns packed uint64_t positions for all cardinal neighbors
     * of (x, y) that contain a pathway tile.
     *
     * @param x    X coordinate.
     * @param y    Y coordinate.
     * @param grid PathwayGrid for pathway tile lookup.
     * @return Vector of packed positions for connected pathway neighbors.
     */
    std::vector<uint64_t> get_pathway_neighbors(int32_t x, int32_t y, const PathwayGrid& grid);
};

} // namespace transport
} // namespace sims3000
