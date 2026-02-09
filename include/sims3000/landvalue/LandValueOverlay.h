/**
 * @file LandValueOverlay.h
 * @brief IGridOverlay implementation for land value visualization (Ticket E10-107)
 *
 * LandValueOverlay wraps a LandValueGrid to provide the IGridOverlay interface.
 * It maps land values (0-255, 128=neutral) to colored tiles for UI rendering:
 * - Very low value (0-63): Red tint, indicating poor land
 * - Low value (64-127): Orange tint, below neutral
 * - Neutral value (128-191): Yellow tint, neutral
 * - High value (192-255): Green tint, premium land
 *
 * The overlay is always active when created.
 */

#ifndef SIMS3000_LANDVALUE_LANDVALUEOVERLAY_H
#define SIMS3000_LANDVALUE_LANDVALUEOVERLAY_H

#include <sims3000/services/IGridOverlay.h>
#include <sims3000/landvalue/LandValueGrid.h>

namespace sims3000 {
namespace landvalue {

/**
 * @class LandValueOverlay
 * @brief IGridOverlay implementation for land value grid visualization.
 *
 * Wraps a LandValueGrid and provides color mapping for UI rendering.
 * Does not own the grid - the grid must outlive the overlay.
 */
class LandValueOverlay : public services::IGridOverlay {
public:
    /**
     * @brief Construct a land value overlay wrapping the given grid.
     *
     * @param grid The land value grid to visualize. Must outlive this overlay.
     */
    explicit LandValueOverlay(const LandValueGrid& grid);

    // IGridOverlay interface
    const char* getName() const override;
    services::OverlayColor getColorAt(uint32_t x, uint32_t y) const override;
    bool isActive() const override;

private:
    const LandValueGrid& m_grid;
};

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_LANDVALUEOVERLAY_H
