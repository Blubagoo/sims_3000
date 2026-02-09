/**
 * @file ServiceCoverageOverlay.h
 * @brief Concrete IGridOverlay for service coverage visualization (Ticket E9-043)
 *
 * ServiceCoverageOverlay maps a ServiceCoverageGrid's per-tile values
 * to colored overlay tiles. The base RGB color is fixed per overlay
 * (e.g. cyan for Enforcer, amber for Hazard), and the alpha channel
 * is set to the grid's coverage value at each tile (0-255).
 *
 * Predefined color schemes:
 * - Enforcer:      cyan/blue  (0, 200, 255)
 * - HazardResponse: amber/orange (255, 180, 0)
 *
 * @see IGridOverlay.h
 * @see ServiceCoverageGrid.h
 */

#pragma once

#include <sims3000/services/IGridOverlay.h>
#include <sims3000/services/ServiceCoverageGrid.h>

namespace sims3000 {
namespace services {

/**
 * @class ServiceCoverageOverlay
 * @brief Renders a ServiceCoverageGrid as a colored overlay.
 *
 * Each tile's color uses the configured base RGB, with alpha set to
 * the grid's coverage value (0-255). This creates a heat-map effect
 * where more coverage = more visible color.
 */
class ServiceCoverageOverlay : public IGridOverlay {
public:
    /**
     * @brief Construct a coverage overlay with a specific color scheme.
     *
     * @param name    Human-readable overlay name (must outlive this object).
     * @param grid    Pointer to the coverage grid to visualize (may be null).
     * @param base_r  Base red channel value (0-255).
     * @param base_g  Base green channel value (0-255).
     * @param base_b  Base blue channel value (0-255).
     */
    ServiceCoverageOverlay(const char* name, const ServiceCoverageGrid* grid,
                           uint8_t base_r, uint8_t base_g, uint8_t base_b);

    /**
     * @brief Get the overlay name.
     * @return The name passed to the constructor.
     */
    const char* getName() const override;

    /**
     * @brief Get the overlay color at a tile.
     *
     * Returns the base RGB with alpha = grid coverage value.
     * Returns {0,0,0,0} if the grid is null or coordinates are out of bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return OverlayColor with base RGB and coverage-based alpha.
     */
    OverlayColor getColorAt(uint32_t x, uint32_t y) const override;

    /**
     * @brief Check if the overlay is active.
     * @return true if setActive(true) has been called.
     */
    bool isActive() const override;

    /**
     * @brief Enable or disable the overlay.
     * @param active true to make the overlay visible.
     */
    void setActive(bool active);

    /**
     * @brief Update the grid pointer (e.g. when grids are rebuilt).
     * @param grid New grid pointer (may be null to disable rendering).
     */
    void setGrid(const ServiceCoverageGrid* grid);

private:
    const char* m_name;                   ///< Overlay display name
    const ServiceCoverageGrid* m_grid;    ///< Source coverage data
    uint8_t m_base_r;                     ///< Base red channel
    uint8_t m_base_g;                     ///< Base green channel
    uint8_t m_base_b;                     ///< Base blue channel
    bool m_active = false;                ///< Whether overlay is visible
};

// ============================================================================
// Predefined Overlay Color Constants
// ============================================================================

/// Enforcer overlay base color: cyan/blue
constexpr uint8_t ENFORCER_OVERLAY_R = 0;
constexpr uint8_t ENFORCER_OVERLAY_G = 200;
constexpr uint8_t ENFORCER_OVERLAY_B = 255;

/// Hazard overlay base color: amber/orange
constexpr uint8_t HAZARD_OVERLAY_R = 255;
constexpr uint8_t HAZARD_OVERLAY_G = 180;
constexpr uint8_t HAZARD_OVERLAY_B = 0;

} // namespace services
} // namespace sims3000
