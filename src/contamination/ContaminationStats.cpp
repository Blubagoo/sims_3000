/**
 * @file ContaminationStats.cpp
 * @brief Implementation of contamination statistics interface (Ticket E10-089)
 *
 * @see ContaminationStats.h
 */

#include <sims3000/contamination/ContaminationStats.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <algorithm>

namespace sims3000 {
namespace contamination {

float get_contamination_stat(const ContaminationGrid& grid, uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_CONTAMINATION: {
            return static_cast<float>(grid.get_total_contamination());
        }

        case STAT_AVERAGE_CONTAMINATION: {
            uint32_t total = grid.get_total_contamination();
            uint32_t cell_count = static_cast<uint32_t>(grid.get_width()) * grid.get_height();
            if (cell_count == 0) {
                return 0.0f;
            }
            return static_cast<float>(total) / static_cast<float>(cell_count);
        }

        case STAT_TOXIC_TILES: {
            return static_cast<float>(grid.get_toxic_tiles());
        }

        case STAT_MAX_CONTAMINATION: {
            uint8_t max_level = 0;
            for (int32_t y = 0; y < grid.get_height(); ++y) {
                for (int32_t x = 0; x < grid.get_width(); ++x) {
                    max_level = std::max(max_level, grid.get_level(x, y));
                }
            }
            return static_cast<float>(max_level);
        }

        case STAT_INDUSTRIAL_TOTAL: {
            uint32_t count = 0;
            for (int32_t y = 0; y < grid.get_height(); ++y) {
                for (int32_t x = 0; x < grid.get_width(); ++x) {
                    if (grid.get_level(x, y) > 0 &&
                        grid.get_dominant_type(x, y) == static_cast<uint8_t>(ContaminationType::Industrial)) {
                        ++count;
                    }
                }
            }
            return static_cast<float>(count);
        }

        case STAT_TRAFFIC_TOTAL: {
            uint32_t count = 0;
            for (int32_t y = 0; y < grid.get_height(); ++y) {
                for (int32_t x = 0; x < grid.get_width(); ++x) {
                    if (grid.get_level(x, y) > 0 &&
                        grid.get_dominant_type(x, y) == static_cast<uint8_t>(ContaminationType::Traffic)) {
                        ++count;
                    }
                }
            }
            return static_cast<float>(count);
        }

        case STAT_ENERGY_TOTAL: {
            uint32_t count = 0;
            for (int32_t y = 0; y < grid.get_height(); ++y) {
                for (int32_t x = 0; x < grid.get_width(); ++x) {
                    if (grid.get_level(x, y) > 0 &&
                        grid.get_dominant_type(x, y) == static_cast<uint8_t>(ContaminationType::Energy)) {
                        ++count;
                    }
                }
            }
            return static_cast<float>(count);
        }

        case STAT_TERRAIN_TOTAL: {
            uint32_t count = 0;
            for (int32_t y = 0; y < grid.get_height(); ++y) {
                for (int32_t x = 0; x < grid.get_width(); ++x) {
                    if (grid.get_level(x, y) > 0 &&
                        grid.get_dominant_type(x, y) == static_cast<uint8_t>(ContaminationType::Terrain)) {
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

uint8_t get_contamination_at(const ContaminationGrid& grid, int32_t x, int32_t y) {
    return grid.get_level(x, y);
}

const char* get_contamination_stat_name(uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_CONTAMINATION:      return "Total Contamination";
        case STAT_AVERAGE_CONTAMINATION:    return "Average Contamination";
        case STAT_TOXIC_TILES:              return "Toxic Tiles";
        case STAT_MAX_CONTAMINATION:        return "Max Contamination";
        case STAT_INDUSTRIAL_TOTAL:         return "Industrial Contamination Tiles";
        case STAT_TRAFFIC_TOTAL:            return "Traffic Contamination Tiles";
        case STAT_ENERGY_TOTAL:             return "Energy Contamination Tiles";
        case STAT_TERRAIN_TOTAL:            return "Terrain Contamination Tiles";
        default:                            return "Unknown";
    }
}

bool is_valid_contamination_stat(uint16_t stat_id) {
    return stat_id >= STAT_TOTAL_CONTAMINATION && stat_id <= STAT_TERRAIN_TOTAL;
}

} // namespace contamination
} // namespace sims3000
