/**
 * @file DataReadoutPanel.h
 * @brief Data readout panel for displaying queried tile/structure information.
 *
 * Provides a query-tool display panel that shows detailed information about
 * a selected tile or structure.  The panel has a compact summary view and
 * an expandable details section showing simulation values and utility
 * connectivity status.
 *
 * Usage:
 * @code
 *   auto panel = std::make_unique<DataReadoutPanel>();
 *   panel->bounds = {10, 100, DataReadoutPanel::PANEL_WIDTH, 200};
 *
 *   TileQueryResult result;
 *   result.position = {42, 17};
 *   result.terrain_type = "Plains";
 *   result.has_structure = true;
 *   result.structure_name = "Relay Hub Alpha";
 *   result.structure_type = "Energy";
 *   result.structure_status = "Active";
 *   panel->show_query(result);
 * @endcode
 */

#ifndef SIMS3000_UI_DATAREADOUTPANEL_H
#define SIMS3000_UI_DATAREADOUTPANEL_H

#include "sims3000/ui/CoreWidgets.h"
#include "sims3000/core/types.h"

#include <optional>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @struct TileQueryResult
 * @brief Information about a queried tile/structure.
 *
 * Populated by the game's query system when the player inspects a tile.
 * Contains terrain, structure, zone, utility, and simulation data for
 * the selected grid position.
 */
struct TileQueryResult {
    /// Grid coordinates of the queried tile
    GridPosition position{0, 0};

    // -- Terrain -------------------------------------------------------------

    /// Terrain type name (e.g. "Plains", "Highlands", "Wetlands")
    std::string terrain_type;

    /// Terrain elevation level (0-255)
    uint8_t elevation = 0;

    // -- Structure (if present) ----------------------------------------------

    /// Whether a structure exists on this tile
    bool has_structure = false;

    /// Display name of the structure
    std::string structure_name;

    /// Category of the structure (e.g. "Energy", "Habitation")
    std::string structure_type;

    /// Current operational status: "Active", "Materializing", "Derelict"
    std::string structure_status;

    // -- Zone ----------------------------------------------------------------

    /// Whether the tile is zoned
    bool has_zone = false;

    /// Zone type name (e.g. "Habitation", "Exchange", "Fabrication")
    std::string zone_type;

    // -- Utilities -----------------------------------------------------------

    /// Whether the tile is connected to the energy network
    bool has_energy = false;

    /// Whether the tile is connected to the fluid network
    bool has_fluid = false;

    /// Distance to nearest pathway tile (255 = no pathway access)
    uint8_t pathway_distance = 255;

    // -- Simulation values ---------------------------------------------------

    /// Disorder level at this tile (0-100)
    uint8_t disorder_level = 0;

    /// Contamination level at this tile (0-100)
    uint8_t contamination_level = 0;

    /// Sector value (desirability) at this tile (0-100)
    uint8_t sector_value = 0;

    // -- Ownership -----------------------------------------------------------

    /// Player who owns this tile (0 = unowned)
    PlayerID owner = 0;
};

/**
 * @class DataReadoutPanel
 * @brief Panel that displays queried tile/structure information.
 *
 * Shows a summary of the selected tile including structure name, type,
 * status, and zone information.  An expandable details section reveals
 * simulation values (sector value, disorder, contamination) and utility
 * connectivity status (energy, fluid, pathway).
 *
 * The panel is positioned by setting its bounds; it uses PANEL_WIDTH as
 * the recommended width and grows vertically based on content.
 */
class DataReadoutPanel : public PanelWidget {
public:
    DataReadoutPanel();

    /**
     * Show the panel with query results for a tile.
     * Makes the panel visible and stores the result for rendering.
     * @param result The tile query data to display
     */
    void show_query(const TileQueryResult& result);

    /**
     * Clear the panel (reset to empty state).
     * The panel remains visible but displays a "No tile selected" message.
     */
    void clear_query();

    /**
     * Check whether the details section is expanded.
     * @return true if the details section is showing
     */
    bool is_details_expanded() const;

    /**
     * Set whether the details section is expanded.
     * @param expanded true to show details, false to collapse
     */
    void set_details_expanded(bool expanded);

    void render(UIRenderer* renderer) override;

    // -- Layout constants ----------------------------------------------------

    /// Recommended panel width in pixels
    static constexpr float PANEL_WIDTH = 280.0f;

    /// Minimum panel height in pixels
    static constexpr float PANEL_MIN_HEIGHT = 120.0f;

    /// Height of each text line in pixels
    static constexpr float LINE_HEIGHT = 20.0f;

private:
    /// Currently displayed query result (nullopt = no selection)
    std::optional<TileQueryResult> m_current_result;

    /// Whether the details section is expanded
    bool m_details_expanded = false;

    // -- Section renderers ---------------------------------------------------

    /**
     * Render the summary section (structure name, type, status, zone).
     * @param renderer The UI renderer
     * @param y_offset Current Y offset within the content area; updated on return
     */
    void render_summary(UIRenderer* renderer, float& y_offset);

    /**
     * Render the details section (sector value, disorder, contamination).
     * @param renderer The UI renderer
     * @param y_offset Current Y offset within the content area; updated on return
     */
    void render_details(UIRenderer* renderer, float& y_offset);

    /**
     * Render the utility status section (energy, fluid, pathway connectivity).
     * @param renderer The UI renderer
     * @param y_offset Current Y offset within the content area; updated on return
     */
    void render_utility_status(UIRenderer* renderer, float& y_offset);

    // -- Colors --------------------------------------------------------------

    /// Color for active/connected status indicators
    static constexpr Color CONNECTED_COLOR() { return {0.0f, 0.8f, 0.0f, 1.0f}; }

    /// Color for disconnected/missing status indicators
    static constexpr Color DISCONNECTED_COLOR() { return {0.8f, 0.2f, 0.2f, 1.0f}; }

    /// Color for section headers
    static constexpr Color HEADER_COLOR() { return {0.7f, 0.8f, 1.0f, 1.0f}; }

    /// Color for normal body text
    static constexpr Color TEXT_COLOR() { return {1.0f, 1.0f, 1.0f, 1.0f}; }

    /// Color for dimmed/secondary text
    static constexpr Color DIM_TEXT_COLOR() { return {0.6f, 0.6f, 0.7f, 1.0f}; }
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_DATAREADOUTPANEL_H
