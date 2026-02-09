/**
 * @file LandValueStats.cpp
 * @brief IStatQueryable implementation for land value statistics (Ticket E10-106)
 */

#include <sims3000/landvalue/LandValueStats.h>
#include <algorithm>

namespace sims3000 {
namespace landvalue {

float get_landvalue_stat(const LandValueGrid& grid, uint16_t stat_id) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();
    const uint32_t total_tiles = static_cast<uint32_t>(width) * height;

    if (total_tiles == 0) {
        return 0.0f;
    }

    switch (stat_id) {
        case STAT_AVERAGE_LAND_VALUE: {
            uint64_t sum = 0;
            for (uint16_t y = 0; y < height; ++y) {
                for (uint16_t x = 0; x < width; ++x) {
                    sum += grid.get_value(x, y);
                }
            }
            return static_cast<float>(sum) / static_cast<float>(total_tiles);
        }

        case STAT_MAX_LAND_VALUE: {
            uint8_t max_value = 0;
            for (uint16_t y = 0; y < height; ++y) {
                for (uint16_t x = 0; x < width; ++x) {
                    max_value = std::max(max_value, grid.get_value(x, y));
                }
            }
            return static_cast<float>(max_value);
        }

        case STAT_MIN_LAND_VALUE: {
            uint8_t min_value = 255;
            for (uint16_t y = 0; y < height; ++y) {
                for (uint16_t x = 0; x < width; ++x) {
                    min_value = std::min(min_value, grid.get_value(x, y));
                }
            }
            return static_cast<float>(min_value);
        }

        case STAT_HIGH_VALUE_TILES: {
            uint32_t count = 0;
            for (uint16_t y = 0; y < height; ++y) {
                for (uint16_t x = 0; x < width; ++x) {
                    if (grid.get_value(x, y) > HIGH_VALUE_THRESHOLD) {
                        ++count;
                    }
                }
            }
            return static_cast<float>(count);
        }

        case STAT_LOW_VALUE_TILES: {
            uint32_t count = 0;
            for (uint16_t y = 0; y < height; ++y) {
                for (uint16_t x = 0; x < width; ++x) {
                    if (grid.get_value(x, y) < LOW_VALUE_THRESHOLD) {
                        ++count;
                    }
                }
            }
            return static_cast<float>(count);
        }

        default:
            return 0.0f;
    }
}

const char* get_landvalue_stat_name(uint16_t stat_id) {
    switch (stat_id) {
        case STAT_AVERAGE_LAND_VALUE:
            return "Average Land Value";

        case STAT_MAX_LAND_VALUE:
            return "Maximum Land Value";

        case STAT_MIN_LAND_VALUE:
            return "Minimum Land Value";

        case STAT_HIGH_VALUE_TILES:
            return "High Value Tiles";

        case STAT_LOW_VALUE_TILES:
            return "Low Value Tiles";

        default:
            return "Unknown";
    }
}

bool is_valid_landvalue_stat(uint16_t stat_id) {
    return stat_id >= STAT_AVERAGE_LAND_VALUE && stat_id <= STAT_LOW_VALUE_TILES;
}

} // namespace landvalue
} // namespace sims3000
