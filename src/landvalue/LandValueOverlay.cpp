/**
 * @file LandValueOverlay.cpp
 * @brief Implementation of land value overlay visualization (Ticket E10-107)
 *
 * @see LandValueOverlay.h
 */

#include <sims3000/landvalue/LandValueOverlay.h>

namespace sims3000 {
namespace landvalue {

LandValueOverlay::LandValueOverlay(const LandValueGrid& grid)
    : m_grid(grid)
{
}

const char* LandValueOverlay::getName() const {
    return "Land Value";
}

services::OverlayColor LandValueOverlay::getColorAt(uint32_t x, uint32_t y) const {
    // Get land value from grid
    uint8_t value = m_grid.get_value(static_cast<int32_t>(x), static_cast<int32_t>(y));

    // Map value to color
    services::OverlayColor color;

    // Land value ranges:
    // 0-63: Very low (red tint)
    // 64-127: Low (orange tint)
    // 128-191: Neutral/good (yellow tint)
    // 192-255: High/premium (green tint)

    if (value < 64) {
        // Very low value - red tint
        color.r = 255;
        color.g = 0;
        color.b = 0;
        color.a = static_cast<uint8_t>(128 + (value * 2));  // Alpha 128-254
    }
    else if (value < 128) {
        // Low value - orange tint
        color.r = 255;
        color.g = 165;
        color.b = 0;
        color.a = static_cast<uint8_t>(128 + ((value - 64) * 2));  // Alpha 128-254
    }
    else if (value < 192) {
        // Neutral value - yellow tint
        color.r = 255;
        color.g = 255;
        color.b = 0;
        color.a = static_cast<uint8_t>(128 + ((value - 128) * 2));  // Alpha 128-254
    }
    else {
        // High value - green tint
        color.r = 0;
        color.g = 255;
        color.b = 0;
        color.a = static_cast<uint8_t>(192 + (value - 192));  // Alpha 192-255
    }

    return color;
}

bool LandValueOverlay::isActive() const {
    return true;
}

} // namespace landvalue
} // namespace sims3000
