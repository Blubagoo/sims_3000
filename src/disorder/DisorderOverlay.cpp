/**
 * @file DisorderOverlay.cpp
 * @brief Implementation of disorder overlay for UI rendering (Ticket E10-078)
 *
 * @see DisorderOverlay.h for public interface documentation.
 */

#include <sims3000/disorder/DisorderOverlay.h>
#include <sims3000/disorder/DisorderGrid.h>

namespace sims3000 {
namespace disorder {

DisorderOverlay::DisorderOverlay(const DisorderGrid& grid)
    : m_grid(grid)
{
}

const char* DisorderOverlay::getName() const {
    return "Disorder";
}

services::OverlayColor DisorderOverlay::getColorAt(uint32_t x, uint32_t y) const {
    // Get disorder level at this position
    uint8_t level = m_grid.get_level(static_cast<int32_t>(x), static_cast<int32_t>(y));

    services::OverlayColor color;

    if (level == 0) {
        // No disorder - fully transparent
        color.r = 0;
        color.g = 0;
        color.b = 0;
        color.a = 0;
    } else if (level <= 85) {
        // Low disorder - green tint
        color.r = 0;
        color.g = 255;
        color.b = 0;
        // Alpha scales with level: 0-85 maps to ~0-85 alpha
        color.a = level;
    } else if (level <= 170) {
        // Medium disorder - yellow tint
        color.r = 255;
        color.g = 255;
        color.b = 0;
        // Alpha scales: 86-170 maps to ~86-170 alpha
        color.a = level;
    } else {
        // High disorder - red tint
        color.r = 255;
        color.g = 0;
        color.b = 0;
        // Alpha scales: 171-255 maps to ~171-255 alpha
        color.a = level;
    }

    return color;
}

bool DisorderOverlay::isActive() const {
    return true;
}

} // namespace disorder
} // namespace sims3000
