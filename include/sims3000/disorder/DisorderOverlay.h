/**
 * @file DisorderOverlay.h
 * @brief IGridOverlay implementation for disorder visualization (Ticket E10-078)
 *
 * DisorderOverlay wraps a DisorderGrid to provide the IGridOverlay interface.
 * It maps disorder levels (0-255) to colored tiles for UI rendering:
 * - Low disorder (0-85): Green tint, low opacity
 * - Medium disorder (86-170): Yellow tint, medium opacity
 * - High disorder (171-255): Red tint, high opacity
 *
 * The overlay is always active when created.
 */

#ifndef SIMS3000_DISORDER_DISORDEROVERLAY_H
#define SIMS3000_DISORDER_DISORDEROVERLAY_H

#include <sims3000/services/IGridOverlay.h>
#include <cstdint>

namespace sims3000 {
namespace disorder {

// Forward declaration
class DisorderGrid;

/**
 * @class DisorderOverlay
 * @brief IGridOverlay implementation for disorder grid visualization.
 *
 * Wraps a DisorderGrid and provides color mapping for UI rendering.
 * Does not own the grid - the grid must outlive the overlay.
 */
class DisorderOverlay : public services::IGridOverlay {
public:
    /**
     * @brief Construct a disorder overlay wrapping the given grid.
     *
     * @param grid The disorder grid to visualize. Must outlive this overlay.
     */
    explicit DisorderOverlay(const DisorderGrid& grid);

    // IGridOverlay interface
    const char* getName() const override;
    services::OverlayColor getColorAt(uint32_t x, uint32_t y) const override;
    bool isActive() const override;

private:
    const DisorderGrid& m_grid;
};

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDEROVERLAY_H
