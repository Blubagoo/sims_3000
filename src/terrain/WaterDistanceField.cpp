/**
 * @file WaterDistanceField.cpp
 * @brief Implementation of multi-source BFS for water distance computation.
 *
 * Uses a deque-based BFS starting from all water tiles simultaneously.
 * This guarantees shortest Manhattan distance to any water tile.
 *
 * Algorithm:
 * 1. Initialize all distances to MAX_WATER_DISTANCE
 * 2. Find all water tiles, set their distance to 0, add to queue
 * 3. BFS: for each tile in queue, check 4-connected neighbors
 *    - If neighbor distance > current + 1 and < 255, update and enqueue
 * 4. All tiles now have their minimum distance to any water tile
 *
 * @see WaterDistanceField.h
 */

#include <sims3000/terrain/WaterDistanceField.h>
#include <deque>

namespace sims3000 {
namespace terrain {

/**
 * @brief Tile coordinate pair for BFS queue.
 */
struct TileCoord {
    std::uint16_t x;
    std::uint16_t y;
};

void WaterDistanceField::compute(const TerrainGrid& terrain) {
    // Ensure field matches terrain dimensions
    if (width != terrain.width || height != terrain.height) {
        width = terrain.width;
        height = terrain.height;
        distances.resize(static_cast<std::size_t>(width) * height);
    }

    // Reset all distances to max
    clear();

    // BFS queue using deque for O(1) push/pop at both ends
    std::deque<TileCoord> queue;

    // Phase 1: Find all water tiles and seed the BFS
    for (std::uint16_t y = 0; y < height; ++y) {
        for (std::uint16_t x = 0; x < width; ++x) {
            const TerrainComponent& tile = terrain.at(x, y);
            if (is_water_type(tile.getTerrainType())) {
                // Water tile: distance 0
                set_distance(x, y, WATER_TILE_DISTANCE);
                queue.push_back({x, y});
            }
        }
    }

    // 4-connected neighbor offsets (Manhattan distance)
    constexpr std::int16_t dx[4] = { 0, 1, 0, -1 };
    constexpr std::int16_t dy[4] = { -1, 0, 1, 0 };

    // Phase 2: BFS from all water tiles simultaneously
    while (!queue.empty()) {
        TileCoord current = queue.front();
        queue.pop_front();

        std::uint8_t current_dist = get_water_distance(current.x, current.y);

        // Don't propagate beyond max distance - 1 (to avoid overflow)
        if (current_dist >= MAX_WATER_DISTANCE - 1) {
            continue;
        }

        std::uint8_t new_dist = static_cast<std::uint8_t>(current_dist + 1);

        // Check all 4 neighbors
        for (int i = 0; i < 4; ++i) {
            std::int32_t nx = static_cast<std::int32_t>(current.x) + dx[i];
            std::int32_t ny = static_cast<std::int32_t>(current.y) + dy[i];

            // Bounds check
            if (!in_bounds(nx, ny)) {
                continue;
            }

            std::uint16_t ux = static_cast<std::uint16_t>(nx);
            std::uint16_t uy = static_cast<std::uint16_t>(ny);

            // Only update if we found a shorter path
            if (get_water_distance(ux, uy) > new_dist) {
                set_distance(ux, uy, new_dist);
                queue.push_back({ux, uy});
            }
        }
    }
}

} // namespace terrain
} // namespace sims3000
