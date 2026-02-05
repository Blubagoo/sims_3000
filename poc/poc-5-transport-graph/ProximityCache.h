#pragma once

#include "PathwayGrid.h"
#include <cstdint>
#include <vector>
#include <queue>

// ProximityCache: Dense 512x512 grid, 1 byte per tile (uint8_t distance, 255 = unreachable)
// Uses flood-fill BFS from all pathway tiles simultaneously for efficient distance calculation.
// Supports the 3-tile rule per interfaces.yaml ITransportProvider

class ProximityCache {
public:
    static constexpr uint8_t UNREACHABLE = 255;

    ProximityCache() = default;

    ProximityCache(uint32_t width, uint32_t height)
        : width_(width), height_(height), data_(width * height, UNREACHABLE) {}

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        data_.assign(width * height, UNREACHABLE);
    }

    // Get distance to nearest pathway (0 = on pathway, 255 = unreachable)
    uint8_t get_distance_to_pathway(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return UNREACHABLE;
        return data_[y * width_ + x];
    }

    // Check if position is accessible (within max_dist tiles of a pathway)
    // Default max_dist = 3 for the 3-tile rule
    bool is_accessible(int32_t x, int32_t y, uint8_t max_dist = 3) const {
        if (!in_bounds(x, y)) return false;
        uint8_t dist = data_[y * width_ + x];
        return dist <= max_dist;
    }

    // Rebuild the cache using multi-source BFS from all pathway tiles
    void rebuild(const PathwayGrid& pathways) {
        // Reset all distances to unreachable
        std::fill(data_.begin(), data_.end(), UNREACHABLE);

        // BFS queue: (x, y)
        std::queue<std::pair<int32_t, int32_t>> queue;

        // Seed BFS with all pathway tiles (distance 0)
        for (int32_t y = 0; y < static_cast<int32_t>(height_); ++y) {
            for (int32_t x = 0; x < static_cast<int32_t>(width_); ++x) {
                if (pathways.has_pathway(x, y)) {
                    data_[y * width_ + x] = 0;
                    queue.push({x, y});
                }
            }
        }

        // 4-connected neighbors (Manhattan distance)
        static const int32_t dx[] = {0, 1, 0, -1};
        static const int32_t dy[] = {-1, 0, 1, 0};

        // BFS flood-fill
        while (!queue.empty()) {
            auto [cx, cy] = queue.front();
            queue.pop();

            uint8_t current_dist = data_[cy * width_ + cx];

            // Don't propagate beyond 254 (keep 255 as unreachable marker)
            if (current_dist >= 254) continue;

            uint8_t new_dist = current_dist + 1;

            for (int i = 0; i < 4; ++i) {
                int32_t nx = cx + dx[i];
                int32_t ny = cy + dy[i];

                if (!in_bounds(nx, ny)) continue;

                size_t idx = ny * width_ + nx;
                if (data_[idx] > new_dist) {
                    data_[idx] = new_dist;
                    queue.push({nx, ny});
                }
            }
        }
    }

    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < static_cast<int32_t>(width_) &&
               y >= 0 && y < static_cast<int32_t>(height_);
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    // Memory info
    size_t memory_bytes() const { return data_.size() * sizeof(uint8_t); }
    size_t bytes_per_tile() const { return sizeof(uint8_t); }

    // Raw data access
    const uint8_t* raw_data() const { return data_.data(); }
    size_t raw_size() const { return data_.size(); }

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<uint8_t> data_;
};
