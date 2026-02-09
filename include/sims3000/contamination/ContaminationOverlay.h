/**
 * @file ContaminationOverlay.h
 * @brief IGridOverlay implementation for contamination visualization (Ticket E10-090)
 *
 * ContaminationOverlay wraps a ContaminationGrid to provide the IGridOverlay interface.
 * It maps contamination levels (0-255) to colored tiles for UI rendering:
 * - Low contamination (0-63): Green tint, low opacity
 * - Medium contamination (64-127): Yellow tint, medium opacity
 * - High contamination (128-191): Orange tint, high opacity
 * - Toxic contamination (192-255): Red tint, very high opacity
 *
 * The overlay is always active when created.
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONOVERLAY_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONOVERLAY_H

#include <sims3000/services/IGridOverlay.h>
#include <cstdint>

namespace sims3000 {
namespace contamination {

// Forward declaration
class ContaminationGrid;

/**
 * @class ContaminationOverlay
 * @brief IGridOverlay implementation for contamination grid visualization.
 *
 * Wraps a ContaminationGrid and provides color mapping for UI rendering.
 * Does not own the grid - the grid must outlive the overlay.
 */
class ContaminationOverlay : public services::IGridOverlay {
public:
    /**
     * @brief Construct a contamination overlay wrapping the given grid.
     *
     * @param grid The contamination grid to visualize. Must outlive this overlay.
     */
    explicit ContaminationOverlay(const ContaminationGrid& grid);

    // IGridOverlay interface
    const char* getName() const override;
    services::OverlayColor getColorAt(uint32_t x, uint32_t y) const override;
    bool isActive() const override;

private:
    const ContaminationGrid& m_grid;
};

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONOVERLAY_H
