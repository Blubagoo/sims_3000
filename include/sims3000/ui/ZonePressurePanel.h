/**
 * @file ZonePressurePanel.h
 * @brief Panel wrapping ZonePressureWidget with title and data binding.
 *
 * Provides a self-contained panel that displays RCI-style zone demand
 * meters.  The panel includes a title bar ("ZONE PRESSURE") and wraps
 * the ZonePressureWidget from ProgressWidgets.h with a data-binding
 * struct (ZoneDemandData) so callers can push demand snapshots without
 * touching the inner widget directly.
 *
 * Usage:
 * @code
 *   auto panel = std::make_unique<ZonePressurePanel>();
 *   panel->bounds = {10, 60, ZonePressurePanel::PANEL_WIDTH,
 *                    ZonePressurePanel::PANEL_HEIGHT};
 *
 *   ZoneDemandData data;
 *   data.habitation = 60;
 *   data.exchange = -20;
 *   data.fabrication = 40;
 *   panel->set_demand(data);
 *
 *   root->add_child(std::move(panel));
 * @endcode
 */

#ifndef SIMS3000_UI_ZONEPRESSUREPANEL_H
#define SIMS3000_UI_ZONEPRESSUREPANEL_H

#include "sims3000/ui/CoreWidgets.h"
#include "sims3000/ui/ProgressWidgets.h"

#include <cstdint>

namespace sims3000 {
namespace ui {

/**
 * @struct ZoneDemandData
 * @brief Demand data from the simulation for zone pressure display.
 *
 * Each field ranges from -100 (oversupply) to +100 (high demand).
 */
struct ZoneDemandData {
    int8_t habitation = 0;    ///< Habitation zone demand (-100 to +100)
    int8_t exchange = 0;      ///< Exchange zone demand (-100 to +100)
    int8_t fabrication = 0;   ///< Fabrication zone demand (-100 to +100)
};

/**
 * @class ZonePressurePanel
 * @brief Panel wrapping ZonePressureWidget with title and data binding.
 *
 * Combines a PanelWidget title bar with a ZonePressureWidget child to
 * create a self-contained zone demand indicator.  The panel title reads
 * "ZONE PRESSURE" per the game's alien terminology.
 */
class ZonePressurePanel : public PanelWidget {
public:
    ZonePressurePanel();

    /**
     * Update with new demand values from a data struct.
     * @param data Snapshot of current zone demand
     */
    void set_demand(const ZoneDemandData& data);

    /**
     * Update with new demand values from individual components.
     * @param hab  Habitation demand (-100 to +100)
     * @param exch Exchange demand (-100 to +100)
     * @param fab  Fabrication demand (-100 to +100)
     */
    void set_demand(int8_t hab, int8_t exch, int8_t fab);

    /**
     * Get the current demand values.
     * @return Const reference to the stored demand data
     */
    const ZoneDemandData& get_demand() const;

    /// Default panel width in pixels
    static constexpr float PANEL_WIDTH = 180.0f;

    /// Default panel height in pixels
    static constexpr float PANEL_HEIGHT = 120.0f;

private:
    ZonePressureWidget* m_pressure_widget = nullptr;
    ZoneDemandData m_data;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_ZONEPRESSUREPANEL_H
