/**
 * @file DisorderStats.cpp
 * @brief Implementation of disorder statistics queries (Ticket E10-077)
 *
 * @see DisorderStats.h for public interface documentation.
 */

#include <sims3000/disorder/DisorderStats.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <algorithm>

namespace sims3000 {
namespace disorder {

float get_disorder_stat(const DisorderGrid& grid, uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_DISORDER:
            return static_cast<float>(grid.get_total_disorder());

        case STAT_AVERAGE_DISORDER: {
            uint32_t total = grid.get_total_disorder();
            uint32_t tile_count = static_cast<uint32_t>(grid.get_width()) * grid.get_height();
            if (tile_count == 0) {
                return 0.0f;
            }
            return static_cast<float>(total) / static_cast<float>(tile_count);
        }

        case STAT_HIGH_DISORDER_TILES:
            return static_cast<float>(grid.get_high_disorder_tiles(128));

        case STAT_MAX_DISORDER: {
            // Scan the grid for the maximum disorder value
            const uint8_t* data = grid.get_raw_data();
            uint32_t size = static_cast<uint32_t>(grid.get_width()) * grid.get_height();
            uint8_t max_value = 0;
            for (uint32_t i = 0; i < size; ++i) {
                if (data[i] > max_value) {
                    max_value = data[i];
                }
            }
            return static_cast<float>(max_value);
        }

        default:
            return 0.0f;
    }
}

const char* get_disorder_stat_name(uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_DISORDER:
            return "Total Disorder";
        case STAT_AVERAGE_DISORDER:
            return "Average Disorder";
        case STAT_HIGH_DISORDER_TILES:
            return "High Disorder Tiles";
        case STAT_MAX_DISORDER:
            return "Max Disorder";
        default:
            return "Unknown";
    }
}

bool is_valid_disorder_stat(uint16_t stat_id) {
    return stat_id == STAT_TOTAL_DISORDER ||
           stat_id == STAT_AVERAGE_DISORDER ||
           stat_id == STAT_HIGH_DISORDER_TILES ||
           stat_id == STAT_MAX_DISORDER;
}

uint8_t get_disorder_at(const DisorderGrid& grid, int32_t x, int32_t y) {
    return grid.get_level(x, y);
}

} // namespace disorder
} // namespace sims3000
