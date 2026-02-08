/**
 * @file FluidCoverageBFS.cpp
 * @brief Implementation of the fluid coverage BFS flood-fill algorithm (Ticket 6-010)
 *
 * Follows the same BFS pattern as EnergySystem::recalculate_coverage() but with
 * fluid-specific seeding logic:
 * - Seeds from OPERATIONAL extractors (powered AND within water proximity)
 * - Seeds from ALL reservoirs (passive storage, no power requirement)
 * - BFS through conduit network with 4-directional traversal
 *
 * @see FluidCoverageBFS.h for API documentation
 * @see EnergySystem.cpp for the energy BFS reference implementation
 */

#include <sims3000/fluid/FluidCoverageBFS.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <sims3000/fluid/FluidEnums.h>
#include <queue>
#include <unordered_set>

namespace sims3000 {
namespace fluid {

// =============================================================================
// mark_coverage_radius
// =============================================================================

void mark_coverage_radius(FluidCoverageGrid& grid,
                          uint32_t cx, uint32_t cy,
                          uint8_t radius, uint8_t owner_id,
                          uint32_t map_width, uint32_t map_height) {
    // Calculate clamped bounds for the square coverage area.
    // Use int64_t to safely handle underflow when cx < radius.
    int64_t min_x = static_cast<int64_t>(cx) - static_cast<int64_t>(radius);
    int64_t min_y = static_cast<int64_t>(cy) - static_cast<int64_t>(radius);
    int64_t max_x = static_cast<int64_t>(cx) + static_cast<int64_t>(radius);
    int64_t max_y = static_cast<int64_t>(cy) + static_cast<int64_t>(radius);

    // Clamp to grid bounds
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= static_cast<int64_t>(map_width)) max_x = static_cast<int64_t>(map_width) - 1;
    if (max_y >= static_cast<int64_t>(map_height)) max_y = static_cast<int64_t>(map_height) - 1;

    // Mark all cells in the clamped square
    for (int64_t y = min_y; y <= max_y; ++y) {
        for (int64_t x = min_x; x <= max_x; ++x) {
            grid.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), owner_id);
        }
    }
}

// =============================================================================
// can_extend_coverage_to (Ticket 6-012)
// =============================================================================

bool can_extend_coverage_to(uint32_t /*x*/, uint32_t /*y*/, uint8_t /*owner*/) {
    // Ownership boundary enforcement stub.
    // Currently always returns true since there is no territory/ownership
    // system yet. When territory boundaries are implemented, this should:
    //   - Return true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
    //   - Return false if tile_owner belongs to a different player
    return true;
}

// =============================================================================
// recalculate_coverage
// =============================================================================

void recalculate_coverage(BFSContext& ctx) {
    // The coverage owner_id is owner+1 (0 means uncovered in the grid)
    uint8_t owner_id = ctx.owner + 1;

    // Step 1: Clear all existing coverage for this owner
    ctx.grid.clear_all_for_owner(owner_id);

    // Step 2: Reset all conduits' is_connected to false for this owner
    if (ctx.registry) {
        for (const auto& pair : ctx.conduit_positions) {
            uint32_t entity_id = pair.second;
            auto entity = static_cast<entt::entity>(entity_id);
            if (ctx.registry->valid(entity)) {
                auto* conduit = ctx.registry->try_get<FluidConduitComponent>(entity);
                if (conduit) {
                    conduit->is_connected = false;
                }
            }
        }
    }

    // BFS frontier and visited set
    std::queue<uint64_t> frontier;
    std::unordered_set<uint64_t> visited;

    // Step 3: Seed from OPERATIONAL extractors
    // Extractors must have is_operational == true (powered AND within water proximity)
    for (const auto& pair : ctx.extractor_positions) {
        uint64_t packed_pos = pair.first;
        uint32_t entity_id = pair.second;

        // Query the FluidProducerComponent to check is_operational
        if (ctx.registry) {
            auto entity = static_cast<entt::entity>(entity_id);
            if (ctx.registry->valid(entity)) {
                const auto* producer = ctx.registry->try_get<FluidProducerComponent>(entity);
                if (!producer || !producer->is_operational) {
                    continue; // Skip non-operational extractors
                }
            } else {
                continue; // Skip invalid entities
            }
        } else {
            continue; // Skip if no registry
        }

        uint32_t ex = unpack_x(packed_pos);
        uint32_t ey = unpack_y(packed_pos);

        // Determine coverage radius from config
        uint8_t radius = EXTRACTOR_DEFAULT_COVERAGE_RADIUS;

        // Mark coverage area around the extractor
        mark_coverage_radius(ctx.grid, ex, ey, radius, owner_id,
                             ctx.map_width, ctx.map_height);

        // Add extractor position to BFS frontier
        if (visited.find(packed_pos) == visited.end()) {
            visited.insert(packed_pos);
            frontier.push(packed_pos);
        }
    }

    // Step 4: Seed from ALL reservoirs (no power check - passive storage)
    for (const auto& pair : ctx.reservoir_positions) {
        uint64_t packed_pos = pair.first;

        uint32_t rx = unpack_x(packed_pos);
        uint32_t ry = unpack_y(packed_pos);

        // Determine coverage radius from config
        uint8_t radius = RESERVOIR_DEFAULT_COVERAGE_RADIUS;

        // Mark coverage area around the reservoir
        mark_coverage_radius(ctx.grid, rx, ry, radius, owner_id,
                             ctx.map_width, ctx.map_height);

        // Add reservoir position to BFS frontier
        if (visited.find(packed_pos) == visited.end()) {
            visited.insert(packed_pos);
            frontier.push(packed_pos);
        }
    }

    // Step 5: BFS through conduit network
    // 4-directional neighbor offsets: right, left, down, up
    static const int32_t dx[] = { 1, -1, 0, 0 };
    static const int32_t dy[] = { 0, 0, 1, -1 };

    while (!frontier.empty()) {
        uint64_t current = frontier.front();
        frontier.pop();

        uint32_t cx = unpack_x(current);
        uint32_t cy = unpack_y(current);

        // Check 4-directional neighbors
        for (int i = 0; i < 4; ++i) {
            int64_t neighbor_x = static_cast<int64_t>(cx) + dx[i];
            int64_t neighbor_y = static_cast<int64_t>(cy) + dy[i];

            // Bounds check
            if (neighbor_x < 0 || neighbor_x >= static_cast<int64_t>(ctx.map_width) ||
                neighbor_y < 0 || neighbor_y >= static_cast<int64_t>(ctx.map_height)) {
                continue;
            }

            uint32_t nbx = static_cast<uint32_t>(neighbor_x);
            uint32_t nby = static_cast<uint32_t>(neighbor_y);
            uint64_t neighbor_key = pack_pos(nbx, nby);

            // Skip if already visited
            if (visited.find(neighbor_key) != visited.end()) {
                continue;
            }

            // Ownership boundary check (Ticket 6-012): verify coverage
            // can extend to this tile for the given owner. Currently always
            // returns true, but provides the integration point for when
            // territory boundaries are implemented.
            if (!can_extend_coverage_to(nbx, nby, ctx.owner)) {
                continue;
            }

            // Check if there's a conduit at this position for this owner
            auto conduit_it = ctx.conduit_positions.find(neighbor_key);
            if (conduit_it == ctx.conduit_positions.end()) {
                continue;
            }

            // Found a conduit - mark it visited and add to frontier
            visited.insert(neighbor_key);
            frontier.push(neighbor_key);

            // Determine conduit coverage radius and mark as connected
            uint8_t conduit_radius = 2; // default from FluidConduitComponent
            if (ctx.registry) {
                auto entity = static_cast<entt::entity>(conduit_it->second);
                if (ctx.registry->valid(entity)) {
                    auto* conduit = ctx.registry->try_get<FluidConduitComponent>(entity);
                    if (conduit) {
                        conduit_radius = conduit->coverage_radius;
                        conduit->is_connected = true;
                    }
                }
            }

            // Mark coverage area around the conduit
            mark_coverage_radius(ctx.grid, nbx, nby, conduit_radius, owner_id,
                                 ctx.map_width, ctx.map_height);
        }
    }
}

} // namespace fluid
} // namespace sims3000
