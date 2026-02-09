/**
 * @file IGridOverlay.h
 * @brief Interface for grid-based coverage visualization overlays (Ticket E9-043)
 *
 * IGridOverlay provides a uniform interface for the UI/render system to
 * query per-tile overlay colors. Each overlay implementation maps its
 * underlying data (e.g. coverage grid values) to RGBA colors.
 *
 * The UISystem can request active overlays and render them as colored
 * tiles on top of the terrain.
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace services {

// ============================================================================
// Overlay Color
// ============================================================================

/**
 * @struct OverlayColor
 * @brief RGBA color for a single overlay tile.
 *
 * Alpha channel typically represents coverage intensity (0 = transparent,
 * 255 = fully opaque).
 */
struct OverlayColor {
    uint8_t r = 0;   ///< Red channel (0-255)
    uint8_t g = 0;   ///< Green channel (0-255)
    uint8_t b = 0;   ///< Blue channel (0-255)
    uint8_t a = 0;   ///< Alpha channel (0-255)
};

// ============================================================================
// Grid Overlay Interface
// ============================================================================

/**
 * @class IGridOverlay
 * @brief Abstract interface for grid-based visualization overlays.
 *
 * Implementations map grid data to per-tile colors for rendering.
 * The UISystem queries active overlays and calls getColorAt() for
 * each visible tile.
 */
class IGridOverlay {
public:
    virtual ~IGridOverlay() = default;

    /**
     * @brief Get the human-readable name of this overlay.
     * @return Null-terminated string name (e.g. "Enforcer Coverage").
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Get the overlay color at a specific tile coordinate.
     *
     * @param x X coordinate (column) of the tile.
     * @param y Y coordinate (row) of the tile.
     * @return OverlayColor for the tile. Returns {0,0,0,0} for out-of-bounds.
     */
    virtual OverlayColor getColorAt(uint32_t x, uint32_t y) const = 0;

    /**
     * @brief Check if this overlay is currently active/visible.
     * @return true if the overlay should be rendered.
     */
    virtual bool isActive() const = 0;
};

} // namespace services
} // namespace sims3000
