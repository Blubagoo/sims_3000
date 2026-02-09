/**
 * @file SliderWidget.cpp
 * @brief Implementation of the SliderWidget and convenience factories.
 *
 * The slider tracks mouse position during a drag and maps the horizontal
 * offset within the track to the configured [min, max] range, snapping
 * to the step increment.  Rendering draws: the track background, a
 * filled portion proportional to the value, a draggable thumb, and the
 * label and numeric value as text.
 */

#include "sims3000/ui/SliderWidget.h"

#include <cmath>
#include <string>

namespace sims3000 {
namespace ui {

// =========================================================================
// Layout constants for internal positioning
// =========================================================================

/// Horizontal space reserved for the label on the left side
static constexpr float LABEL_WIDTH = 160.0f;

/// Horizontal space reserved for the value text on the right side
static constexpr float VALUE_WIDTH = 50.0f;

/// Horizontal padding between label/value and the track
static constexpr float PADDING = 8.0f;

// =========================================================================
// Construction
// =========================================================================

SliderWidget::SliderWidget(float min_value, float max_value, float step,
                           const std::string& label)
    : m_value(min_value)
    , m_min_value(min_value)
    , m_max_value(max_value)
    , m_step(step)
    , label(label) {
}

// =========================================================================
// Value access
// =========================================================================

void SliderWidget::set_value(float v) {
    m_value = clamp_and_snap(v);
}

float SliderWidget::get_value() const {
    return m_value;
}

void SliderWidget::set_on_change(std::function<void(float)> callback) {
    m_on_change = callback;
}

// =========================================================================
// Widget overrides
// =========================================================================

void SliderWidget::update(float delta_time) {
    // Update children (base class behavior)
    Widget::update(delta_time);
}

void SliderWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    Rect track = get_track_rect();

    // -- Label (left side) ---------------------------------------------------

    float label_x = screen_bounds.x;
    float label_y = screen_bounds.y
                    + (screen_bounds.height - 14.0f) * 0.5f;
    renderer->draw_text(label, label_x, label_y,
                        FontSize::Small, LABEL_COLOR());

    // -- Track background ----------------------------------------------------

    renderer->draw_rect(track, TRACK_COLOR(), TRACK_BORDER_COLOR());

    // -- Filled portion ------------------------------------------------------

    float norm = get_normalized();
    float fill_width = track.width * norm;

    if (fill_width > 0.0f) {
        Rect fill_rect = {
            track.x,
            track.y,
            fill_width,
            track.height
        };
        Color fill_color = (m_dragging || is_hovered())
                               ? FILL_ACTIVE_COLOR()
                               : FILL_COLOR();
        renderer->draw_rect(fill_rect, fill_color, fill_color);
    }

    // -- Thumb ---------------------------------------------------------------

    float thumb_center_x = track.x + fill_width;
    float thumb_x = thumb_center_x - THUMB_WIDTH * 0.5f;
    float thumb_y = track.y + track.height * 0.5f - THUMB_HEIGHT * 0.5f;

    Rect thumb_rect = {
        thumb_x,
        thumb_y,
        THUMB_WIDTH,
        THUMB_HEIGHT
    };

    Color thumb_color = m_dragging ? THUMB_DRAG_COLOR() : THUMB_COLOR();
    renderer->draw_rect(thumb_rect, thumb_color, THUMB_BORDER_COLOR());

    // -- Value text (right side) ---------------------------------------------

    float value_x = track.x + track.width + PADDING;
    float value_y = screen_bounds.y
                    + (screen_bounds.height - 14.0f) * 0.5f;
    renderer->draw_text(format_value(), value_x, value_y,
                        FontSize::Small, VALUE_COLOR());

    // -- Delegate to draw_slider for renderer-level effects ------------------

    renderer->draw_slider(track, m_value, m_min_value, m_max_value);

    // Render children
    Widget::render(renderer);
}

void SliderWidget::on_mouse_down(int button, float x, float y) {
    if (button != 0 || !enabled) {
        return;
    }

    Rect track = get_track_rect();

    // Extend the hit area vertically to include the thumb height
    float hit_top = track.y - (THUMB_HEIGHT - track.height) * 0.5f;
    float hit_bottom = hit_top + THUMB_HEIGHT;

    if (x >= track.x - THUMB_WIDTH * 0.5f
        && x <= track.x + track.width + THUMB_WIDTH * 0.5f
        && y >= hit_top
        && y <= hit_bottom) {

        m_dragging = true;
        set_pressed(true);

        // Immediately jump to the clicked position
        float new_value = screen_x_to_value(x);
        if (new_value != m_value) {
            m_value = new_value;
            if (m_on_change) {
                m_on_change(m_value);
            }
        }
    }
}

void SliderWidget::on_mouse_up(int button, float /*x*/, float /*y*/) {
    if (button == 0) {
        m_dragging = false;
        set_pressed(false);
    }
}

void SliderWidget::on_mouse_move(float x, float /*y*/) {
    if (!m_dragging) {
        return;
    }

    float new_value = screen_x_to_value(x);
    if (new_value != m_value) {
        m_value = new_value;
        if (m_on_change) {
            m_on_change(m_value);
        }
    }
}

// =========================================================================
// Internal helpers
// =========================================================================

float SliderWidget::clamp_and_snap(float v) const {
    // Clamp to range
    if (v < m_min_value) {
        v = m_min_value;
    }
    if (v > m_max_value) {
        v = m_max_value;
    }

    // Snap to nearest step
    if (m_step > 0.0f) {
        float offset = v - m_min_value;
        float steps = std::round(offset / m_step);
        v = m_min_value + steps * m_step;

        // Re-clamp after snapping (rounding could push past max)
        if (v > m_max_value) {
            v = m_max_value;
        }
    }

    return v;
}

float SliderWidget::screen_x_to_value(float screen_x) const {
    Rect track = get_track_rect();

    if (track.width <= 0.0f) {
        return m_min_value;
    }

    float t = (screen_x - track.x) / track.width;

    // Clamp t to [0, 1]
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    float raw_value = m_min_value + t * (m_max_value - m_min_value);
    return clamp_and_snap(raw_value);
}

float SliderWidget::get_normalized() const {
    float range = m_max_value - m_min_value;
    if (range <= 0.0f) {
        return 0.0f;
    }
    float norm = (m_value - m_min_value) / range;
    if (norm < 0.0f) {
        norm = 0.0f;
    }
    if (norm > 1.0f) {
        norm = 1.0f;
    }
    return norm;
}

std::string SliderWidget::format_value() const {
    int int_value = static_cast<int>(m_value + 0.5f);
    return std::to_string(int_value) + "%";
}

Rect SliderWidget::get_track_rect() const {
    float track_x = screen_bounds.x + LABEL_WIDTH + PADDING;
    float track_width = screen_bounds.width
                        - LABEL_WIDTH - PADDING
                        - VALUE_WIDTH - PADDING;

    if (track_width < 0.0f) {
        track_width = 0.0f;
    }

    float track_y = screen_bounds.y
                    + (screen_bounds.height - TRACK_HEIGHT) * 0.5f;

    return Rect{track_x, track_y, track_width, TRACK_HEIGHT};
}

// =========================================================================
// Convenience factories
// =========================================================================

std::unique_ptr<SliderWidget> create_tribute_slider(
    const std::string& label,
    std::function<void(float)> callback) {

    auto slider = std::make_unique<SliderWidget>(0.0f, 20.0f, 1.0f, label);
    slider->set_on_change(callback);
    return slider;
}

std::unique_ptr<SliderWidget> create_funding_slider(
    const std::string& label,
    std::function<void(float)> callback) {

    auto slider = std::make_unique<SliderWidget>(0.0f, 150.0f, 5.0f, label);
    slider->set_value(100.0f);  // Default funding at 100%
    slider->set_on_change(callback);
    return slider;
}

} // namespace ui
} // namespace sims3000
