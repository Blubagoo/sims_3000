/**
 * @file BoundaryFlags.h
 * @brief Boundary flags for rendering ownership edges (Epic 7, Ticket E7-028)
 *
 * Header-only utility for generating ownership boundary flags for pathway
 * rendering. Each pathway tile gets flags indicating which edges border
 * a tile owned by a different player.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TransportEnums.h>
#include <cstdint>
#include <functional>

namespace sims3000 {
namespace transport {

/**
 * @struct PathwayRenderData
 * @brief Pathway data prepared for rendering, including boundary flags.
 */
struct PathwayRenderData {
    int32_t x, y;                  ///< Grid position
    PathwayType type;              ///< Pathway type (road, corridor, etc.)
    uint8_t health;                ///< Health/condition (0-255)
    uint8_t congestion_level;      ///< Congestion severity (0-255)
    uint8_t owner;                 ///< Owner player ID (0-3)
    uint8_t boundary_flags;        ///< N(1), S(2), E(4), W(8) - set if neighbor has different owner
};

/**
 * @brief Calculate boundary flags for a pathway at (x,y) owned by 'owner'.
 *
 * Checks each cardinal neighbor. If the neighbor has a different non-zero
 * owner, the corresponding flag bit is set:
 * - Bit 0 (1): North neighbor has different owner
 * - Bit 1 (2): South neighbor has different owner
 * - Bit 2 (4): East neighbor has different owner
 * - Bit 3 (8): West neighbor has different owner
 *
 * A neighbor with owner=0 (no pathway) does NOT set the boundary flag.
 *
 * @param x         X coordinate of the pathway tile.
 * @param y         Y coordinate of the pathway tile.
 * @param owner     Owner player ID of this tile.
 * @param owner_at  Function returning owner_id at a given position (0 = no pathway).
 * @return Boundary flags bitmask.
 */
inline uint8_t calculate_boundary_flags(
    int32_t x, int32_t y, uint8_t owner,
    const std::function<uint8_t(int32_t, int32_t)>& owner_at)
{
    uint8_t flags = 0;
    auto check = [&](int32_t nx, int32_t ny, uint8_t flag) {
        uint8_t neighbor_owner = owner_at(nx, ny);
        if (neighbor_owner != 0 && neighbor_owner != owner) {
            flags |= flag;
        }
    };
    check(x, y - 1, 1); // North
    check(x, y + 1, 2); // South
    check(x + 1, y, 4); // East
    check(x - 1, y, 8); // West
    return flags;
}

} // namespace transport
} // namespace sims3000
