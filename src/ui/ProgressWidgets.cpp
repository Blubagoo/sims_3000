/**
 * @file ProgressWidgets.cpp
 * @brief Implementation of ProgressBarWidget and ZonePressureWidget.
 */

#include "sims3000/ui/ProgressWidgets.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace ui {

// =========================================================================
// ProgressBarWidget
// =========================================================================

void ProgressBarWidget::update(float delta_time) {
    // Smooth interpolation toward target value
    float blend = std::min(LERP_SPEED * delta_time, 1.0f);
    value += (target_value - value) * blend;
    value = std::max(0.0f, std::min(1.0f, value));

    Widget::update(delta_time);
}

void ProgressBarWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // Background
    Color border_color = {0.3f, 0.3f, 0.35f, 1.0f};
    renderer->draw_rect(screen_bounds, background_color, border_color);

    // Filled portion
    if (value > 0.0f) {
        Rect fill_bounds = screen_bounds;
        fill_bounds.width *= value;
        renderer->draw_rect(fill_bounds, fill_color, fill_color);
    }

    // Optional centered label
    if (show_label && !label_text.empty()) {
        float text_x = screen_bounds.x + screen_bounds.width * 0.5f;
        float text_y = screen_bounds.y + screen_bounds.height * 0.25f;
        Color label_color = {1.0f, 1.0f, 1.0f, 1.0f};
        renderer->draw_text(label_text, text_x, text_y, FontSize::Small, label_color);
    }

    // Render children
    Widget::render(renderer);
}

void ProgressBarWidget::set_value(float v) {
    target_value = std::max(0.0f, std::min(1.0f, v));
}

void ProgressBarWidget::set_value_immediate(float v) {
    float clamped = std::max(0.0f, std::min(1.0f, v));
    value = clamped;
    target_value = clamped;
}

// =========================================================================
// ZonePressureWidget
// =========================================================================

void ZonePressureWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // Title text at the top
    Color title_color = {1.0f, 1.0f, 1.0f, 1.0f};
    renderer->draw_text("ZONE PRESSURE", screen_bounds.x, screen_bounds.y,
                        FontSize::Small, title_color);

    // Reserve space for the title row
    constexpr float title_height = 18.0f;
    float remaining_height = screen_bounds.height - title_height;
    float bar_height = remaining_height / 3.0f;

    float bar_y = screen_bounds.y + title_height;

    // Habitation bar
    Rect hab_bounds = {screen_bounds.x, bar_y, screen_bounds.width, bar_height};
    render_demand_bar(renderer, hab_bounds, habitation_demand,
                      HABITATION_COLOR(), "H");

    // Exchange bar
    Rect exc_bounds = {screen_bounds.x, bar_y + bar_height, screen_bounds.width, bar_height};
    render_demand_bar(renderer, exc_bounds, exchange_demand,
                      EXCHANGE_COLOR(), "E");

    // Fabrication bar
    Rect fab_bounds = {screen_bounds.x, bar_y + bar_height * 2.0f, screen_bounds.width, bar_height};
    render_demand_bar(renderer, fab_bounds, fabrication_demand,
                      FABRICATION_COLOR(), "F");

    // Render children
    Widget::render(renderer);
}

void ZonePressureWidget::render_demand_bar(
    UIRenderer* renderer, const Rect& bar_bounds,
    int8_t demand, const Color& positive_color,
    const std::string& label)
{
    // Background
    Color bg_color = {0.15f, 0.15f, 0.2f, 1.0f};
    Color border_color = {0.3f, 0.3f, 0.35f, 1.0f};
    renderer->draw_rect(bar_bounds, bg_color, border_color);

    // Calculate fill from the center
    float fraction = std::abs(static_cast<float>(demand)) / 100.0f;
    float center_x = bar_bounds.x + bar_bounds.width * 0.5f;
    float half_width = bar_bounds.width * 0.5f;
    float fill_width = half_width * fraction;

    if (demand > 0) {
        // Fill from center to right with positive color
        Rect fill = {center_x, bar_bounds.y, fill_width, bar_bounds.height};
        renderer->draw_rect(fill, positive_color, positive_color);
    } else if (demand < 0) {
        // Fill from center to left with negative color
        Rect fill = {center_x - fill_width, bar_bounds.y, fill_width, bar_bounds.height};
        renderer->draw_rect(fill, NEGATIVE_COLOR(), NEGATIVE_COLOR());
    }

    // Label on left side (e.g. "H", "E", "F")
    Color label_color = {1.0f, 1.0f, 1.0f, 1.0f};
    renderer->draw_text(label, bar_bounds.x + 2.0f, bar_bounds.y + 1.0f,
                        FontSize::Small, label_color);

    // Numeric value on right side
    std::string value_text = std::to_string(static_cast<int>(demand));
    float text_x = bar_bounds.x + bar_bounds.width - 24.0f;
    renderer->draw_text(value_text, text_x, bar_bounds.y + 1.0f,
                        FontSize::Small, label_color);
}

} // namespace ui
} // namespace sims3000
