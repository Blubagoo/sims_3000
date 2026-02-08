/**
 * @file ProximityCache.cpp
 * @brief Implementation of ProximityCache multi-source BFS distance cache.
 *        (Epic 7, Ticket E7-006)
 *
 * @see ProximityCache.h for class documentation.
 */

#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/PathwayGrid.h>
#include <queue>

namespace sims3000 {
namespace transport {

ProximityCache::ProximityCache(uint32_t width, uint32_t height)
    : distance_cache_(static_cast<size_t>(width) * height, 255)
    , dirty_(true)
    , width_(width)
    , height_(height)
{
}

// ============================================================================
// Distance queries
// ============================================================================

uint8_t ProximityCache::get_distance(int32_t x, int32_t y) const {
    if (x < 0 || y < 0
        || static_cast<uint32_t>(x) >= width_
        || static_cast<uint32_t>(y) >= height_) {
        return 255;
    }
    return distance_cache_[static_cast<size_t>(y) * width_ + static_cast<size_t>(x)];
}

// ============================================================================
// Rebuild
// ============================================================================

void ProximityCache::rebuild_if_dirty(const PathwayGrid& grid) {
    if (!dirty_) {
        return;
    }
    rebuild(grid);
}

void ProximityCache::rebuild(const PathwayGrid& grid) {
    // Resize cache if grid dimensions changed
    const size_t total = static_cast<size_t>(width_) * height_;

    // Initialize all distances to 255
    for (size_t i = 0; i < total; ++i) {
        distance_cache_[i] = 255;
    }

    // Multi-source BFS: seed with all pathway tiles at distance 0
    // Use a simple queue of (x, y) packed into uint32_t pairs
    std::queue<std::pair<int32_t, int32_t>> frontier;

    for (int32_t y = 0; y < static_cast<int32_t>(height_); ++y) {
        for (int32_t x = 0; x < static_cast<int32_t>(width_); ++x) {
            if (grid.has_pathway(x, y)) {
                distance_cache_[static_cast<size_t>(y) * width_ + static_cast<size_t>(x)] = 0;
                frontier.push({x, y});
            }
        }
    }

    // 4-directional offsets (N/S/E/W)
    static const int32_t dx[] = { 0, 0, 1, -1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    // BFS expansion
    while (!frontier.empty()) {
        auto [cx, cy] = frontier.front();
        frontier.pop();

        uint8_t current_dist = distance_cache_[static_cast<size_t>(cy) * width_ + static_cast<size_t>(cx)];

        // Cap at 254 to prevent overflow (next distance would be 255 which is the sentinel)
        if (current_dist >= 254) {
            continue;
        }

        uint8_t next_dist = current_dist + 1;

        for (int i = 0; i < 4; ++i) {
            int32_t nx = cx + dx[i];
            int32_t ny = cy + dy[i];

            if (nx < 0 || ny < 0
                || static_cast<uint32_t>(nx) >= width_
                || static_cast<uint32_t>(ny) >= height_) {
                continue;
            }

            size_t idx = static_cast<size_t>(ny) * width_ + static_cast<size_t>(nx);
            if (distance_cache_[idx] > next_dist) {
                distance_cache_[idx] = next_dist;
                frontier.push({nx, ny});
            }
        }
    }

    dirty_ = false;
}

// ============================================================================
// Dirty tracking
// ============================================================================

void ProximityCache::mark_dirty() {
    dirty_ = true;
}

bool ProximityCache::is_dirty() const {
    return dirty_;
}

// ============================================================================
// Dimensions
// ============================================================================

uint32_t ProximityCache::width() const {
    return width_;
}

uint32_t ProximityCache::height() const {
    return height_;
}

} // namespace transport
} // namespace sims3000
