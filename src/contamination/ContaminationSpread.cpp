/**
 * @file ContaminationSpread.cpp
 * @brief Implementation of contamination spread algorithm.
 *
 * @see ContaminationSpread.h for algorithm documentation.
 * @see E10-087
 */

#include <sims3000/contamination/ContaminationSpread.h>
#include <vector>
#include <cstdint>

namespace sims3000 {
namespace contamination {

namespace {

/**
 * @brief Cardinal direction offsets (N, S, E, W).
 */
constexpr int32_t CARDINAL_OFFSETS[4][2] = {
    {0, -1},  // North
    {0, 1},   // South
    {1, 0},   // East
    {-1, 0}   // West
};

/**
 * @brief Diagonal direction offsets (NE, NW, SE, SW).
 */
constexpr int32_t DIAGONAL_OFFSETS[4][2] = {
    {1, -1},  // Northeast
    {-1, -1}, // Northwest
    {1, 1},   // Southeast
    {-1, 1}   // Southwest
};

/**
 * @brief Structure to hold spread delta for a single cell.
 */
struct SpreadDelta {
    uint8_t amount = 0;
    uint8_t type = 0;
};

} // anonymous namespace

void apply_contamination_spread(ContaminationGrid& grid) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();
    const size_t grid_size = static_cast<size_t>(width) * height;

    // Delta buffer to accumulate spread amounts
    std::vector<SpreadDelta> deltas(grid_size);

    // Phase 1: Calculate all spread amounts and accumulate in delta buffer
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            // Read from previous tick buffer
            const uint8_t level = grid.get_level_previous_tick(x, y);

            // Skip if below threshold
            if (level < CONTAM_SPREAD_THRESHOLD) {
                continue;
            }

            const uint8_t dominant_type = grid.get_dominant_type_previous_tick(x, y);

            // Calculate spread amounts
            const uint8_t cardinal_spread = level / 8;
            const uint8_t diagonal_spread = level / 16;

            // Spread to cardinal neighbors
            for (int i = 0; i < 4; ++i) {
                const int32_t nx = x + CARDINAL_OFFSETS[i][0];
                const int32_t ny = y + CARDINAL_OFFSETS[i][1];

                if (grid.is_valid(nx, ny) && cardinal_spread > 0) {
                    const size_t idx = static_cast<size_t>(ny) * width + static_cast<size_t>(nx);

                    // Accumulate spread amount (saturate at 255)
                    const uint16_t new_amount = static_cast<uint16_t>(deltas[idx].amount) + cardinal_spread;
                    deltas[idx].amount = (new_amount > 255) ? 255 : static_cast<uint8_t>(new_amount);

                    // Use the dominant type from the spreading cell
                    // (In case of multiple sources, the last one wins - simple resolution)
                    deltas[idx].type = dominant_type;
                }
            }

            // Spread to diagonal neighbors
            for (int i = 0; i < 4; ++i) {
                const int32_t nx = x + DIAGONAL_OFFSETS[i][0];
                const int32_t ny = y + DIAGONAL_OFFSETS[i][1];

                if (grid.is_valid(nx, ny) && diagonal_spread > 0) {
                    const size_t idx = static_cast<size_t>(ny) * width + static_cast<size_t>(nx);

                    // Accumulate spread amount (saturate at 255)
                    const uint16_t new_amount = static_cast<uint16_t>(deltas[idx].amount) + diagonal_spread;
                    deltas[idx].amount = (new_amount > 255) ? 255 : static_cast<uint8_t>(new_amount);

                    deltas[idx].type = dominant_type;
                }
            }
        }
    }

    // Phase 2: Apply all deltas to the current buffer
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            const size_t idx = static_cast<size_t>(y) * width + static_cast<size_t>(x);
            const SpreadDelta& delta = deltas[idx];

            if (delta.amount > 0) {
                grid.add_contamination(x, y, delta.amount, delta.type);
            }
        }
    }
}

} // namespace contamination
} // namespace sims3000
