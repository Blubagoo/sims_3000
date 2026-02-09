/**
 * @file ColonyStatusBar.h
 * @brief Always-visible status bar showing key colony information.
 *
 * Displays population count, treasury balance, current date (cycle/phase),
 * and simulation speed at the top of the screen.  All text uses alien
 * terminology (beings, cycles, phases) consistent with the game's theme.
 *
 * Usage:
 * @code
 *   auto bar = std::make_unique<ColonyStatusBar>();
 *   bar->bounds = {0, 0, 1280, ColonyStatusBar::BAR_HEIGHT};
 *
 *   ColonyStatusData data;
 *   data.population = 12450;
 *   data.treasury_balance = 45230;
 *   data.current_cycle = 5;
 *   data.current_phase = 3;
 *   bar->set_data(data);
 *
 *   root->add_child(std::move(bar));
 * @endcode
 */

#ifndef SIMS3000_UI_COLONYSTATUSBAR_H
#define SIMS3000_UI_COLONYSTATUSBAR_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"

#include <cstdint>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @struct ColonyStatusData
 * @brief Snapshot of colony statistics for the status bar display.
 */
struct ColonyStatusData {
    int64_t population = 0;         ///< Total number of beings in the colony
    int64_t treasury_balance = 0;   ///< Current treasury balance in credits
    uint32_t current_cycle = 0;     ///< Current cycle ("year" in alien terms)
    uint32_t current_phase = 0;     ///< Current phase ("month" in alien terms)
    bool paused = false;            ///< Whether the simulation is paused
    int speed_multiplier = 1;       ///< Speed: 1=normal, 2=fast, 3=ultra
};

/**
 * @class ColonyStatusBar
 * @brief Always-visible status bar showing key colony info.
 *
 * Renders a horizontal bar at the top of the screen with population,
 * treasury, date, and speed indicator.  The bar is semi-transparent
 * so the game world remains partially visible behind it.
 */
class ColonyStatusBar : public Widget {
public:
    ColonyStatusBar();

    /**
     * Update the displayed colony data.
     * @param data Snapshot of current colony statistics
     */
    void set_data(const ColonyStatusData& data);

    /**
     * Get the current colony data.
     * @return Const reference to the stored data
     */
    const ColonyStatusData& get_data() const;

    void render(UIRenderer* renderer) override;

    /// Height of the status bar in pixels
    static constexpr float BAR_HEIGHT = 28.0f;

private:
    ColonyStatusData m_data;

    /**
     * Format population with thousands separators.
     * @param pop Population count
     * @return Formatted string, e.g. "12,450"
     */
    static std::string format_population(int64_t pop);

    /**
     * Format treasury balance with thousands separators and currency.
     * @param balance Treasury balance
     * @return Formatted string, e.g. "45,230 cr"
     */
    static std::string format_treasury(int64_t balance);

    /**
     * Format the current date in alien terminology.
     * @param cycle Current cycle number
     * @param phase Current phase number
     * @return Formatted string, e.g. "Cycle 5, Phase 3"
     */
    static std::string format_date(uint32_t cycle, uint32_t phase);

    /**
     * Format the speed indicator.
     * @param paused Whether the simulation is paused
     * @param speed  Speed multiplier (1, 2, or 3)
     * @return Formatted string, e.g. "[>]", "[>>]", "[>>>]", or "[PAUSED]"
     */
    static std::string format_speed(bool paused, int speed);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_COLONYSTATUSBAR_H
