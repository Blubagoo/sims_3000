/**
 * @file ProgressWidgets.h
 * @brief Progress bar and zone demand meter widgets.
 *
 * Provides widgets for displaying numeric progress and zone demand:
 * - ProgressBarWidget: Horizontal bar with smooth fill animation
 * - ZonePressureWidget: RCI-style demand meter with three demand bars
 *
 * Usage:
 * @code
 *   auto bar = std::make_unique<ProgressBarWidget>();
 *   bar->bounds = {10, 10, 200, 24};
 *   bar->set_value(0.75f);  // smooth animation to 75%
 *
 *   auto rci = std::make_unique<ZonePressureWidget>();
 *   rci->bounds = {10, 50, 120, 100};
 *   rci->habitation_demand = 60;
 *   rci->exchange_demand = -20;
 *   rci->fabrication_demand = 40;
 * @endcode
 */

#ifndef SIMS3000_UI_PROGRESSWIDGETS_H
#define SIMS3000_UI_PROGRESSWIDGETS_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"

#include <cstdint>
#include <string>

namespace sims3000 {
namespace ui {

// =========================================================================
// ProgressBarWidget
// =========================================================================

/**
 * @class ProgressBarWidget
 * @brief Horizontal progress bar with smooth value animation.
 *
 * Displays a filled bar representing a value from 0.0 to 1.0.  When
 * set_value() is called the bar smoothly interpolates toward the target;
 * set_value_immediate() snaps to the new value instantly.  An optional
 * text label can be displayed centered on the bar.
 */
class ProgressBarWidget : public Widget {
public:
    /// Current displayed value (0.0 - 1.0)
    float value = 0.0f;

    /// Target value for smooth animation (0.0 - 1.0)
    float target_value = 0.0f;

    /// Color of the filled portion
    Color fill_color = {0.0f, 0.8f, 0.8f, 1.0f};

    /// Color of the unfilled background
    Color background_color = {0.15f, 0.15f, 0.2f, 1.0f};

    /// Whether to display the label text on top of the bar
    bool show_label = false;

    /// Text to display when show_label is true
    std::string label_text;

    void update(float delta_time) override;
    void render(UIRenderer* renderer) override;

    /**
     * Set the target value with smooth animation.
     * @param v Desired value, clamped to [0.0, 1.0]
     */
    void set_value(float v);

    /**
     * Set the value immediately without animation.
     * @param v Desired value, clamped to [0.0, 1.0]
     */
    void set_value_immediate(float v);

    /// Animation speed (units per second)
    static constexpr float LERP_SPEED = 5.0f;
};

// =========================================================================
// ZonePressureWidget
// =========================================================================

/**
 * @class ZonePressureWidget
 * @brief RCI-style zone demand meter showing three demand bars.
 *
 * Displays Habitation (green), Exchange (blue), and Fabrication (yellow)
 * demand bars.  Each demand value ranges from -100 to +100.  Positive
 * demand fills from center to right in the zone color; negative demand
 * fills from center to left in red.
 */
class ZonePressureWidget : public Widget {
public:
    /// Habitation zone demand (-100 to +100)
    int8_t habitation_demand = 0;

    /// Exchange zone demand (-100 to +100)
    int8_t exchange_demand = 0;

    /// Fabrication zone demand (-100 to +100)
    int8_t fabrication_demand = 0;

    void render(UIRenderer* renderer) override;

    /// Green color for habitation zone bars
    static constexpr Color HABITATION_COLOR() { return {0.0f, 0.8f, 0.0f, 1.0f}; }

    /// Blue color for exchange zone bars
    static constexpr Color EXCHANGE_COLOR() { return {0.0f, 0.4f, 0.8f, 1.0f}; }

    /// Yellow color for fabrication zone bars
    static constexpr Color FABRICATION_COLOR() { return {0.8f, 0.8f, 0.0f, 1.0f}; }

    /// Red color for negative demand
    static constexpr Color NEGATIVE_COLOR() { return {0.8f, 0.2f, 0.2f, 1.0f}; }

private:
    /**
     * Render a single demand bar within the given bounds.
     * @param renderer       The renderer to draw with
     * @param bar_bounds     Screen-space rectangle for this bar row
     * @param demand         Demand value (-100 to +100)
     * @param positive_color Color used when demand is positive
     * @param label          Short label (e.g. "H", "E", "F")
     */
    void render_demand_bar(UIRenderer* renderer, const Rect& bar_bounds,
                           int8_t demand, const Color& positive_color,
                           const std::string& label);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_PROGRESSWIDGETS_H
