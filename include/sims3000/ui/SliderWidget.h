/**
 * @file SliderWidget.h
 * @brief Slider widget for tribute rates and service funding levels.
 *
 * Provides a draggable horizontal slider with configurable range, step
 * snapping, and numeric value display.  Includes convenience factories
 * for the two standard slider configurations used in the budget panel:
 *
 * - Tribute rate sliders: 0-20%, 1% step
 * - Funding level sliders: 0-150%, 5% step
 *
 * Also defines event structs (UITributeRateChangedEvent and
 * UIFundingChangedEvent) for propagating slider changes to game logic.
 *
 * Usage:
 * @code
 *   auto slider = create_tribute_slider("Habitation Tribute",
 *       [](float rate) { set_tribute(0, rate); });
 *   slider->set_value(9.0f);
 *   panel->add_child(std::move(slider));
 * @endcode
 */

#ifndef SIMS3000_UI_SLIDERWIDGET_H
#define SIMS3000_UI_SLIDERWIDGET_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace sims3000 {
namespace ui {

// =========================================================================
// Event structs
// =========================================================================

/**
 * @struct UITributeRateChangedEvent
 * @brief Emitted when a tribute rate slider is adjusted.
 */
struct UITributeRateChangedEvent {
    uint8_t zone_type;  ///< Zone type identifier (0=Habitation, 1=Exchange, 2=Fabrication)
    float new_rate;     ///< New tribute rate as a percentage (0.0 - 20.0)
};

/**
 * @struct UIFundingChangedEvent
 * @brief Emitted when a service funding slider is adjusted.
 */
struct UIFundingChangedEvent {
    uint8_t service_type;  ///< Service type identifier
    uint8_t new_level;     ///< New funding percentage (0-150)
};

// =========================================================================
// SliderWidget
// =========================================================================

/**
 * @class SliderWidget
 * @brief Horizontal slider widget with drag interaction and value snapping.
 *
 * The slider renders a horizontal track with a filled portion indicating
 * the current value relative to the range, a draggable thumb, and a text
 * label showing the current numeric value.
 *
 * During a drag operation the slider captures mouse movement and maps the
 * horizontal position to the value range, snapping to the configured step
 * increment.  The on_change callback fires whenever the value changes.
 */
class SliderWidget : public Widget {
public:
    /**
     * Construct a slider widget.
     * @param min_value Minimum value of the slider range
     * @param max_value Maximum value of the slider range
     * @param step      Step increment for snapping
     * @param label     Display label shown beside the slider
     */
    SliderWidget(float min_value, float max_value, float step,
                 const std::string& label);

    // -- Value access --------------------------------------------------------

    /**
     * Set the slider value, clamping to range and snapping to step.
     * @param v The desired value
     */
    void set_value(float v);

    /**
     * Get the current slider value.
     * @return Current value within [min_value, max_value]
     */
    float get_value() const;

    /**
     * Set the callback invoked when the value changes via user drag.
     * @param callback Function receiving the new value
     */
    void set_on_change(std::function<void(float)> callback);

    // -- Widget overrides ----------------------------------------------------

    void update(float delta_time) override;
    void render(UIRenderer* renderer) override;

    void on_mouse_down(int button, float x, float y) override;
    void on_mouse_up(int button, float x, float y) override;
    void on_mouse_move(float x, float y) override;

    // -- Public state --------------------------------------------------------

    /// Display label shown to the left of the slider
    std::string label;

    // -- Layout constants ----------------------------------------------------

    /// Height of the slider track in pixels
    static constexpr float TRACK_HEIGHT = 8.0f;

    /// Width of the draggable thumb in pixels
    static constexpr float THUMB_WIDTH = 12.0f;

    /// Height of the draggable thumb in pixels
    static constexpr float THUMB_HEIGHT = 20.0f;

private:
    float m_value;
    float m_min_value;
    float m_max_value;
    float m_step;
    bool m_dragging = false;

    /// Callback fired on value change during drag
    std::function<void(float)> m_on_change;

    /**
     * Clamp a value to [min, max] and snap to the nearest step.
     * @param v Raw value
     * @return Clamped and snapped value
     */
    float clamp_and_snap(float v) const;

    /**
     * Map a screen-space X coordinate to a slider value.
     * @param screen_x The X position in screen space
     * @return The corresponding clamped/snapped value
     */
    float screen_x_to_value(float screen_x) const;

    /**
     * Compute the normalized position (0.0 - 1.0) of the current value.
     * @return Normalized position within the slider range
     */
    float get_normalized() const;

    /**
     * Format the current value as a percentage string (e.g. "12%").
     * @return Formatted string
     */
    std::string format_value() const;

    /**
     * Get the screen-space rectangle of the slider track area.
     * The track is positioned within screen_bounds, leaving room for
     * the label on the left and the value text on the right.
     * @return Rect of the track in screen space
     */
    Rect get_track_rect() const;

    // -- Rendering colors ----------------------------------------------------

    /// Color of the unfilled slider track
    static constexpr Color TRACK_COLOR() { return {0.2f, 0.22f, 0.3f, 1.0f}; }

    /// Color of the filled portion of the track
    static constexpr Color FILL_COLOR() { return {0.3f, 0.5f, 0.8f, 1.0f}; }

    /// Color of the fill when hovered or being dragged
    static constexpr Color FILL_ACTIVE_COLOR() { return {0.4f, 0.6f, 0.9f, 1.0f}; }

    /// Color of the draggable thumb
    static constexpr Color THUMB_COLOR() { return {0.7f, 0.75f, 0.85f, 1.0f}; }

    /// Color of the thumb when being dragged
    static constexpr Color THUMB_DRAG_COLOR() { return {0.9f, 0.92f, 1.0f, 1.0f}; }

    /// Color of the track border
    static constexpr Color TRACK_BORDER_COLOR() { return {0.35f, 0.4f, 0.5f, 1.0f}; }

    /// Color of the thumb border
    static constexpr Color THUMB_BORDER_COLOR() { return {0.5f, 0.55f, 0.65f, 1.0f}; }

    /// Color for the label text
    static constexpr Color LABEL_COLOR() { return {0.7f, 0.8f, 1.0f, 1.0f}; }

    /// Color for the value text
    static constexpr Color VALUE_COLOR() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
};

// =========================================================================
// Convenience factories
// =========================================================================

/**
 * Create a tribute rate slider (0-20%, 1% step).
 * @param label    Display label (e.g. "Habitation Tribute")
 * @param callback Function invoked with the new rate when the slider changes
 * @return Unique pointer to the configured SliderWidget
 */
std::unique_ptr<SliderWidget> create_tribute_slider(
    const std::string& label,
    std::function<void(float)> callback);

/**
 * Create a service funding slider (0-150%, 5% step).
 * @param label    Display label (e.g. "Enforcer Funding")
 * @param callback Function invoked with the new level when the slider changes
 * @return Unique pointer to the configured SliderWidget
 */
std::unique_ptr<SliderWidget> create_funding_slider(
    const std::string& label,
    std::function<void(float)> callback);

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SLIDERWIDGET_H
