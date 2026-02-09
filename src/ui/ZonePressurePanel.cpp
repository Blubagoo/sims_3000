/**
 * @file ZonePressurePanel.cpp
 * @brief Implementation of ZonePressurePanel: zone demand indicator panel.
 */

#include "sims3000/ui/ZonePressurePanel.h"

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

ZonePressurePanel::ZonePressurePanel() {
    title = "ZONE PRESSURE";
    closable = false;
    draggable = false;

    bounds = {0.0f, 0.0f, PANEL_WIDTH, PANEL_HEIGHT};

    // Create the ZonePressureWidget as a child, positioned inside the
    // content area (below the title bar).
    auto pressure = std::make_unique<ZonePressureWidget>();
    pressure->bounds = {
        0.0f,
        PanelWidget::TITLE_BAR_HEIGHT,
        PANEL_WIDTH,
        PANEL_HEIGHT - PanelWidget::TITLE_BAR_HEIGHT
    };

    m_pressure_widget = static_cast<ZonePressureWidget*>(
        add_child(std::move(pressure)));
}

// =========================================================================
// Data binding
// =========================================================================

void ZonePressurePanel::set_demand(const ZoneDemandData& data) {
    m_data = data;
    if (m_pressure_widget) {
        m_pressure_widget->habitation_demand = data.habitation;
        m_pressure_widget->exchange_demand = data.exchange;
        m_pressure_widget->fabrication_demand = data.fabrication;
    }
}

void ZonePressurePanel::set_demand(int8_t hab, int8_t exch, int8_t fab) {
    m_data.habitation = hab;
    m_data.exchange = exch;
    m_data.fabrication = fab;
    if (m_pressure_widget) {
        m_pressure_widget->habitation_demand = hab;
        m_pressure_widget->exchange_demand = exch;
        m_pressure_widget->fabrication_demand = fab;
    }
}

const ZoneDemandData& ZonePressurePanel::get_demand() const {
    return m_data;
}

} // namespace ui
} // namespace sims3000
