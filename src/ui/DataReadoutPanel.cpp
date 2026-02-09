/**
 * @file DataReadoutPanel.cpp
 * @brief Implementation of the Data Readout Panel (query tool display).
 *
 * Renders queried tile/structure information in a collapsible panel:
 * - Summary: position, terrain, structure name/type/status, zone
 * - Details: sector value, disorder, contamination levels
 * - Utilities: energy, fluid, pathway connectivity status
 */

#include "sims3000/ui/DataReadoutPanel.h"

#include <string>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

DataReadoutPanel::DataReadoutPanel() {
    title = "DATA READOUT";
    closable = true;
    bounds.width = PANEL_WIDTH;
    bounds.height = PANEL_MIN_HEIGHT;
}

// =========================================================================
// Public interface
// =========================================================================

void DataReadoutPanel::show_query(const TileQueryResult& result) {
    m_current_result = result;
    visible = true;
}

void DataReadoutPanel::clear_query() {
    m_current_result.reset();
    m_details_expanded = false;
}

bool DataReadoutPanel::is_details_expanded() const {
    return m_details_expanded;
}

void DataReadoutPanel::set_details_expanded(bool expanded) {
    m_details_expanded = expanded;
}

// =========================================================================
// Rendering
// =========================================================================

void DataReadoutPanel::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // Draw the panel chrome (background, title bar, border)
    renderer->draw_panel(screen_bounds, title, closable);

    // Content area starts below the title bar
    Rect content = get_content_bounds();
    float y_offset = content.y + 4.0f;
    float left_margin = content.x + 8.0f;

    if (!m_current_result.has_value()) {
        // Empty state
        renderer->draw_text("No tile selected", left_margin, y_offset,
                            FontSize::Normal, DIM_TEXT_COLOR());
    } else {
        render_summary(renderer, y_offset);

        if (m_details_expanded) {
            render_details(renderer, y_offset);
            render_utility_status(renderer, y_offset);
        } else {
            // Show expand hint
            renderer->draw_text("[+] Details...", left_margin, y_offset,
                                FontSize::Small, DIM_TEXT_COLOR());
            y_offset += LINE_HEIGHT;
        }
    }

    // Render children (close button, etc.)
    Widget::render(renderer);
}

// =========================================================================
// Section: Summary
// =========================================================================

void DataReadoutPanel::render_summary(UIRenderer* renderer, float& y_offset) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 8.0f;

    const TileQueryResult& r = m_current_result.value();

    // Position coordinates
    std::string pos_text = "Grid: (" + std::to_string(r.position.x) + ", "
                           + std::to_string(r.position.y) + ")";
    renderer->draw_text(pos_text, left_margin, y_offset,
                        FontSize::Small, DIM_TEXT_COLOR());
    y_offset += LINE_HEIGHT;

    // Terrain type and elevation
    std::string terrain_text = "Terrain: " + r.terrain_type
                               + " (Elev " + std::to_string(static_cast<int>(r.elevation)) + ")";
    renderer->draw_text(terrain_text, left_margin, y_offset,
                        FontSize::Small, TEXT_COLOR());
    y_offset += LINE_HEIGHT;

    // Structure info (if present)
    if (r.has_structure) {
        // Structure name as a heading
        renderer->draw_text(r.structure_name, left_margin, y_offset,
                            FontSize::Large, HEADER_COLOR());
        y_offset += LINE_HEIGHT + 2.0f;

        // Type
        std::string type_text = "Type: " + r.structure_type;
        renderer->draw_text(type_text, left_margin, y_offset,
                            FontSize::Normal, TEXT_COLOR());
        y_offset += LINE_HEIGHT;

        // Status with color coding
        Color status_color = TEXT_COLOR();
        if (r.structure_status == "Active") {
            status_color = CONNECTED_COLOR();
        } else if (r.structure_status == "Derelict") {
            status_color = DISCONNECTED_COLOR();
        } else if (r.structure_status == "Materializing") {
            status_color = Color{0.8f, 0.8f, 0.0f, 1.0f};  // Yellow
        }

        std::string status_text = "Status: " + r.structure_status;
        renderer->draw_text(status_text, left_margin, y_offset,
                            FontSize::Normal, status_color);
        y_offset += LINE_HEIGHT;
    } else {
        renderer->draw_text("No structure", left_margin, y_offset,
                            FontSize::Normal, DIM_TEXT_COLOR());
        y_offset += LINE_HEIGHT;
    }

    // Zone info (if present)
    if (r.has_zone) {
        std::string zone_text = "Zone: " + r.zone_type;
        renderer->draw_text(zone_text, left_margin, y_offset,
                            FontSize::Normal, TEXT_COLOR());
        y_offset += LINE_HEIGHT;
    }

    // Separator line
    y_offset += 4.0f;
}

// =========================================================================
// Section: Details (expanded)
// =========================================================================

void DataReadoutPanel::render_details(UIRenderer* renderer, float& y_offset) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 8.0f;

    const TileQueryResult& r = m_current_result.value();

    // Section header
    renderer->draw_text("[-] SECTOR ANALYSIS", left_margin, y_offset,
                        FontSize::Small, HEADER_COLOR());
    y_offset += LINE_HEIGHT;

    // Sector value
    std::string sv_text = "Sector Value: "
                          + std::to_string(static_cast<int>(r.sector_value)) + "/100";
    renderer->draw_text(sv_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, TEXT_COLOR());
    y_offset += LINE_HEIGHT;

    // Disorder level
    std::string disorder_text = "Disorder: "
                                + std::to_string(static_cast<int>(r.disorder_level)) + "/100";
    Color disorder_color = TEXT_COLOR();
    if (r.disorder_level > 50) {
        disorder_color = DISCONNECTED_COLOR();
    } else if (r.disorder_level > 25) {
        disorder_color = Color{0.8f, 0.8f, 0.0f, 1.0f};  // Yellow warning
    }
    renderer->draw_text(disorder_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, disorder_color);
    y_offset += LINE_HEIGHT;

    // Contamination level
    std::string contam_text = "Contamination: "
                              + std::to_string(static_cast<int>(r.contamination_level)) + "/100";
    Color contam_color = TEXT_COLOR();
    if (r.contamination_level > 50) {
        contam_color = DISCONNECTED_COLOR();
    } else if (r.contamination_level > 25) {
        contam_color = Color{0.8f, 0.8f, 0.0f, 1.0f};  // Yellow warning
    }
    renderer->draw_text(contam_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, contam_color);
    y_offset += LINE_HEIGHT;

    // Ownership
    if (r.owner > 0) {
        std::string owner_text = "Owner: Player " + std::to_string(static_cast<int>(r.owner));
        renderer->draw_text(owner_text, left_margin + 8.0f, y_offset,
                            FontSize::Normal, TEXT_COLOR());
        y_offset += LINE_HEIGHT;
    }

    // Separator
    y_offset += 4.0f;
}

// =========================================================================
// Section: Utility Status (expanded)
// =========================================================================

void DataReadoutPanel::render_utility_status(UIRenderer* renderer, float& y_offset) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 8.0f;

    const TileQueryResult& r = m_current_result.value();

    // Section header
    renderer->draw_text("UTILITY STATUS", left_margin, y_offset,
                        FontSize::Small, HEADER_COLOR());
    y_offset += LINE_HEIGHT;

    // Energy
    std::string energy_text = "Energy: ";
    Color energy_color;
    if (r.has_energy) {
        energy_text += "Connected";
        energy_color = CONNECTED_COLOR();
    } else {
        energy_text += "Disconnected";
        energy_color = DISCONNECTED_COLOR();
    }
    renderer->draw_text(energy_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, energy_color);
    y_offset += LINE_HEIGHT;

    // Fluid
    std::string fluid_text = "Fluid: ";
    Color fluid_color;
    if (r.has_fluid) {
        fluid_text += "Connected";
        fluid_color = CONNECTED_COLOR();
    } else {
        fluid_text += "Disconnected";
        fluid_color = DISCONNECTED_COLOR();
    }
    renderer->draw_text(fluid_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, fluid_color);
    y_offset += LINE_HEIGHT;

    // Pathway access
    std::string pathway_text = "Pathway: ";
    Color pathway_color;
    if (r.pathway_distance < 255) {
        pathway_text += "Dist " + std::to_string(static_cast<int>(r.pathway_distance));
        pathway_color = CONNECTED_COLOR();
    } else {
        pathway_text += "No Access";
        pathway_color = DISCONNECTED_COLOR();
    }
    renderer->draw_text(pathway_text, left_margin + 8.0f, y_offset,
                        FontSize::Normal, pathway_color);
    y_offset += LINE_HEIGHT;
}

} // namespace ui
} // namespace sims3000
