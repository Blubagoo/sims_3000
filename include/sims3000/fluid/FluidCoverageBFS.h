/**
 * @file FluidCoverageBFS.h
 * @brief Fluid coverage BFS flood-fill algorithm for Epic 6 (Ticket 6-010)
 *
 * Provides the BFS flood-fill algorithm for computing fluid coverage zones.
 * Separated from FluidSystem to allow parallel development (6-009 builds
 * the FluidSystem skeleton, 6-010 builds the BFS helper).
 *
 * Algorithm overview:
 * 1. Clear existing coverage for the owner
 * 2. Reset all conduits' is_connected to false
 * 3. Seed from OPERATIONAL extractors (must be powered AND within water proximity)
 * 4. Seed from ALL reservoirs (passive storage, no power requirement)
 * 5. BFS through 4-directional conduit network
 * 6. Each discovered conduit marks its coverage_radius and is added to frontier
 *
 * Key differences from energy BFS:
 * - Seeds from operational extractors (not nexuses)
 * - Seeds from ALL reservoirs regardless of power state
 * - Uses FluidProducerComponent.is_operational instead of always-seed
 *
 * Performance target: <10ms for 512x512 with 5,000 conduits
 *
 * @see EnergySystem.cpp recalculate_coverage() for the energy BFS pattern
 * @see FluidCoverageGrid.h for the grid API
 * @see FluidConduitComponent.h for conduit data
 * @see FluidProducerComponent.h for producer data
 */

#pragma once

#include <sims3000/fluid/FluidCoverageGrid.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <unordered_map>

namespace sims3000 {
namespace fluid {

/**
 * @struct BFSContext
 * @brief Context data passed to the fluid coverage BFS algorithm.
 *
 * Aggregates all the data needed by recalculate_coverage() so it can
 * operate as a standalone helper without needing access to FluidSystem
 * internals directly.
 *
 * Position maps use packed 64-bit keys: (x << 32) | y
 */
struct BFSContext {
    FluidCoverageGrid& grid;                                        ///< Coverage grid to write to
    const std::unordered_map<uint64_t, uint32_t>& extractor_positions;  ///< Packed(x,y) -> entity_id for extractors
    const std::unordered_map<uint64_t, uint32_t>& reservoir_positions;  ///< Packed(x,y) -> entity_id for reservoirs
    const std::unordered_map<uint64_t, uint32_t>& conduit_positions;    ///< Packed(x,y) -> entity_id for conduits
    entt::registry* registry;                                       ///< ECS registry for component queries
    uint8_t owner;                                                  ///< Player ID (0-3)
    uint32_t map_width;                                             ///< Map width in tiles
    uint32_t map_height;                                            ///< Map height in tiles
};

/**
 * @brief Pack two 32-bit coordinates into a single 64-bit key.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @return Packed 64-bit key (x in upper 32 bits, y in lower 32 bits).
 */
inline uint64_t pack_pos(uint32_t x, uint32_t y) {
    return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
}

/**
 * @brief Unpack X coordinate from a packed 64-bit position key.
 * @param packed The packed position key.
 * @return The X coordinate.
 */
inline uint32_t unpack_x(uint64_t packed) {
    return static_cast<uint32_t>(packed >> 32);
}

/**
 * @brief Unpack Y coordinate from a packed 64-bit position key.
 * @param packed The packed position key.
 * @return The Y coordinate.
 */
inline uint32_t unpack_y(uint64_t packed) {
    return static_cast<uint32_t>(packed & 0xFFFFFFFF);
}

/**
 * @brief Mark a square coverage area around a center point.
 *
 * Marks all cells within the square [cx-radius, cx+radius] x
 * [cy-radius, cy+radius] as covered by the given owner. Clamps
 * to grid bounds automatically.
 *
 * @param grid The coverage grid to modify.
 * @param cx Center X coordinate.
 * @param cy Center Y coordinate.
 * @param radius Coverage radius in tiles.
 * @param owner_id Coverage owner (1-4, where owner_id = player_id + 1).
 * @param map_width Map width for bounds clamping.
 * @param map_height Map height for bounds clamping.
 */
void mark_coverage_radius(FluidCoverageGrid& grid,
                          uint32_t cx, uint32_t cy,
                          uint8_t radius, uint8_t owner_id,
                          uint32_t map_width, uint32_t map_height);

/**
 * @brief Check if coverage can extend to a tile for a given owner.
 *
 * Ownership boundary enforcement stub for Ticket 6-012.
 * Currently always returns true since there is no territory/ownership
 * system yet.
 *
 * Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed).
 * Returns false if tile_owner belongs to a different player.
 *
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @param owner Player ID (0-3).
 * @return true if coverage can extend to this tile (stub: always true).
 */
bool can_extend_coverage_to(uint32_t x, uint32_t y, uint8_t owner);

/**
 * @brief Recalculate fluid coverage for a specific player via BFS flood-fill.
 *
 * Algorithm:
 * 1. Clear all existing coverage for this owner.
 * 2. Reset all conduits' is_connected to false for this owner.
 * 3. Seed BFS from OPERATIONAL extractors:
 *    - Query FluidProducerComponent.is_operational == true
 *    - Mark coverage_radius around each operational extractor
 *    - Add extractor position to BFS queue
 * 4. Seed BFS from ALL reservoirs (no power check - passive storage):
 *    - Mark coverage_radius around each reservoir
 *    - Add reservoir position to BFS queue
 * 5. BFS through 4-directional conduit network:
 *    - For each position in queue, check up/down/left/right neighbors
 *    - If neighbor has a conduit owned by same player:
 *      - Set conduit is_connected = true
 *      - Mark coverage_radius around conduit position
 *      - Add conduit position to queue
 * 6. Continue until queue is empty.
 *
 * Performance: O(conduits), not O(grid cells).
 * Target: <10ms for 512x512 with 5,000 conduits.
 *
 * @param ctx BFS context containing all required data.
 */
void recalculate_coverage(BFSContext& ctx);

} // namespace fluid
} // namespace sims3000
