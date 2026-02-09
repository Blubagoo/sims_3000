/**
 * @file DisorderSpread.cpp
 * @brief Implementation of disorder spread algorithm using delta buffer.
 *
 * @see DisorderSpread.h for function documentation.
 * @see E10-075
 */

#include <sims3000/disorder/DisorderSpread.h>
#include <algorithm>
#include <cstdint>

namespace sims3000 {
namespace disorder {

void apply_disorder_spread(DisorderGrid& grid, const std::vector<bool>* water_mask) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();
    const size_t total_cells = static_cast<size_t>(width) * height;

    // Step 1: Create delta buffer (signed to allow negative deltas for source cells)
    std::vector<int16_t> delta(total_cells, 0);

    // 4-neighbor offsets: right, left, down, up
    const int32_t dx[] = { 1, -1, 0, 0 };
    const int32_t dy[] = { 0, 0, 1, -1 };

    // Step 2: Calculate deltas
    for (int32_t y = 0; y < static_cast<int32_t>(height); ++y) {
        for (int32_t x = 0; x < static_cast<int32_t>(width); ++x) {
            uint8_t level = grid.get_level(x, y);
            if (level <= SPREAD_THRESHOLD) {
                continue;
            }

            uint8_t spread = static_cast<uint8_t>((level - SPREAD_THRESHOLD) / 8);
            if (spread == 0) {
                continue;
            }

            size_t src_idx = static_cast<size_t>(y) * width + static_cast<size_t>(x);
            int num_valid_neighbors = 0;

            for (int d = 0; d < 4; ++d) {
                int32_t nx = x + dx[d];
                int32_t ny = y + dy[d];

                if (!grid.is_valid(nx, ny)) {
                    continue;
                }

                // Check water mask
                if (water_mask != nullptr) {
                    size_t neighbor_idx = static_cast<size_t>(ny) * width + static_cast<size_t>(nx);
                    if ((*water_mask)[neighbor_idx]) {
                        continue;
                    }
                }

                size_t neighbor_idx = static_cast<size_t>(ny) * width + static_cast<size_t>(nx);
                delta[neighbor_idx] += static_cast<int16_t>(spread);
                ++num_valid_neighbors;
            }

            // Source loses spread * num_valid_neighbors
            delta[src_idx] -= static_cast<int16_t>(spread) * static_cast<int16_t>(num_valid_neighbors);
        }
    }

    // Step 3: Apply deltas with clamping to [0, 255]
    for (int32_t y = 0; y < static_cast<int32_t>(height); ++y) {
        for (int32_t x = 0; x < static_cast<int32_t>(width); ++x) {
            size_t idx = static_cast<size_t>(y) * width + static_cast<size_t>(x);
            int16_t d = delta[idx];
            if (d == 0) {
                continue;
            }

            int16_t current = static_cast<int16_t>(grid.get_level(x, y));
            int16_t new_level = current + d;

            // Clamp to [0, 255]
            if (new_level < 0) {
                new_level = 0;
            } else if (new_level > 255) {
                new_level = 255;
            }

            grid.set_level(x, y, static_cast<uint8_t>(new_level));
        }
    }
}

} // namespace disorder
} // namespace sims3000
