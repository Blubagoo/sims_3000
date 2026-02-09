/**
 * @file ContaminationOverlay.cpp
 * @brief Implementation of contamination overlay visualization (Ticket E10-090)
 *
 * @see ContaminationOverlay.h
 */

#include <sims3000/contamination/ContaminationOverlay.h>
#include <sims3000/contamination/ContaminationGrid.h>

namespace sims3000 {
namespace contamination {

ContaminationOverlay::ContaminationOverlay(const ContaminationGrid& grid)
    : m_grid(grid)
{
}

const char* ContaminationOverlay::getName() const {
    return "Contamination";
}

services::OverlayColor ContaminationOverlay::getColorAt(uint32_t x, uint32_t y) const {
    // Get contamination level from grid
    uint8_t level = m_grid.get_level(static_cast<int32_t>(x), static_cast<int32_t>(y));

    // Map level to color
    services::OverlayColor color;

    if (level == 0) {
        // No contamination - transparent
        color.r = 0;
        color.g = 0;
        color.b = 0;
        color.a = 0;
    }
    else if (level < 64) {
        // Low contamination - green tint
        color.r = 0;
        color.g = 255;
        color.b = 0;
        color.a = static_cast<uint8_t>(64 + (level * 2));  // Alpha 64-190
    }
    else if (level < 128) {
        // Medium contamination - yellow tint
        color.r = 255;
        color.g = 255;
        color.b = 0;
        color.a = static_cast<uint8_t>(128 + (level - 64));  // Alpha 128-191
    }
    else if (level < 192) {
        // High contamination - orange tint
        color.r = 255;
        color.g = 165;
        color.b = 0;
        color.a = static_cast<uint8_t>(160 + ((level - 128) / 2));  // Alpha 160-191
    }
    else {
        // Toxic contamination - red tint
        color.r = 255;
        color.g = 0;
        color.b = 0;
        color.a = static_cast<uint8_t>(192 + ((level - 192) / 4));  // Alpha 192-207
    }

    return color;
}

bool ContaminationOverlay::isActive() const {
    return true;
}

} // namespace contamination
} // namespace sims3000
